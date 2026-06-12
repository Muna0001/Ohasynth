/*
 * Juno.Engine — headless synth engine.
 *
 * Owns the AudioContext + AudioWorklet node and the current patch state.
 * The UI talks to the engine only through this API, so the engine can be
 * reused without any DOM:
 *
 *   const e = new Juno.Engine();
 *   await e.start();            // must be called from a user gesture
 *   e.noteOn(60, 0.8); e.noteOff(60);
 *   e.setParam('vcfFreq', 0.4);
 *   e.loadPatch(Juno.PRESETS[0]);
 */
(function () {
  'use strict';
  window.Juno = window.Juno || {};

  // Parameter schema. All sliders are normalized 0..1; switches are small
  // integers. Mapping to real units happens inside the worklet.
  Juno.PARAMS = {
    lfoRate:    { def: 0.40, type: 'slider' },
    lfoDelay:   { def: 0.00, type: 'slider' },
    dcoRange:   { def: 1,    type: 'enum', n: 3 },   // 0=16' 1=8' 2=4'
    dcoLfo:     { def: 0.00, type: 'slider' },
    dcoPw:      { def: 0.00, type: 'slider' },
    dcoPwmMode: { def: 0,    type: 'enum', n: 2 },   // 0=MAN 1=LFO
    dcoSaw:     { def: 1,    type: 'enum', n: 2 },
    dcoPulse:   { def: 0,    type: 'enum', n: 2 },
    dcoSub:     { def: 0.00, type: 'slider' },
    dcoNoise:   { def: 0.00, type: 'slider' },
    hpf:        { def: 1,    type: 'enum', n: 4 },
    vcfFreq:    { def: 0.55, type: 'slider' },
    vcfRes:     { def: 0.00, type: 'slider' },
    vcfPol:     { def: 0,    type: 'enum', n: 2 },   // 0=normal 1=inverted
    vcfEnv:     { def: 0.25, type: 'slider' },
    vcfLfo:     { def: 0.00, type: 'slider' },
    vcfKey:     { def: 0.50, type: 'slider' },
    vcaMode:    { def: 0,    type: 'enum', n: 2 },   // 0=ENV 1=GATE
    vcaLevel:   { def: 0.80, type: 'slider' },
    envA:       { def: 0.05, type: 'slider' },
    envD:       { def: 0.40, type: 'slider' },
    envS:       { def: 0.70, type: 'slider' },
    envR:       { def: 0.30, type: 'slider' },
    chorus:     { def: 0,    type: 'enum', n: 4 },   // 0=off 1=I 2=II 3=I+II
    bendDco:    { def: 0.1667, type: 'slider' },     // 0..1 => 0..12 semitones
    bendVcf:    { def: 0.00, type: 'slider' },
    velSens:    { def: 0.30, type: 'slider' },
    volume:     { def: 0.75, type: 'slider' }
  };

  Juno.defaultPatch = function () {
    var p = { name: 'INIT' };
    for (var k in Juno.PARAMS) p[k] = Juno.PARAMS[k].def;
    return p;
  };

  Juno.Engine = class {
    constructor() {
      this.ctx = null;
      this.node = null;
      this.patch = Juno.defaultPatch();
      this.held = new Set();
      this.sustained = new Set();
      this.sustainOn = false;
      this._listeners = {};
      this._pending = [];      // messages sent before audio is ready
      this._startPromise = null;
    }

    on(ev, fn) {
      (this._listeners[ev] = this._listeners[ev] || []).push(fn);
    }
    _emit(ev) {
      var args = Array.prototype.slice.call(arguments, 1);
      (this._listeners[ev] || []).forEach(function (fn) { fn.apply(null, args); });
    }

    get running() { return !!this.ctx && this.ctx.state === 'running'; }

    // Must be called from a user gesture (browser autoplay policy).
    // Safe to call repeatedly; concurrent calls share one startup.
    start() {
      if (this._startPromise) {
        if (this.ctx && this.ctx.state !== 'running') this.ctx.resume();
        return this._startPromise;
      }
      var self = this;
      this._startPromise = this._doStart().catch(function (err) {
        self._startPromise = null; // allow a retry on the next gesture
        console.error('Oh-a-synth: audio failed to start:', err);
        throw err;
      });
      return this._startPromise;
    }

    async _doStart() {
      var ctx = new (window.AudioContext || window.webkitAudioContext)({
        latencyHint: 'interactive'
      });
      var code = '(' + Juno.workletMain.toString() + ')();';

      // Preferred: AudioWorklet (audio-thread DSP). Module loading needs a
      // URL; blob URLs can be refused on file:// origins in some browsers,
      // so fall back to a data: URL, then to a main-thread ScriptProcessor
      // shim that runs the exact same processor code.
      var node = null;
      if (ctx.audioWorklet) {
        var blobUrl = URL.createObjectURL(new Blob([code], { type: 'application/javascript' }));
        node = await this._tryWorklet(ctx, blobUrl);
        URL.revokeObjectURL(blobUrl);
        if (!node)
          node = await this._tryWorklet(ctx,
            'data:application/javascript;charset=utf-8,' + encodeURIComponent(code));
      }
      if (!node) node = this._makeScriptProcessorNode(ctx, code);

      node.connect(ctx.destination);
      this.node = node;
      this.ctx = ctx;
      if (ctx.state !== 'running') await ctx.resume();

      this._send({ type: 'patch', values: this._patchValues() });
      var pending = this._pending;
      this._pending = [];
      for (var i = 0; i < pending.length; i++) this._send(pending[i]);
      this._emit('started');
    }

    async _tryWorklet(ctx, url) {
      try {
        await ctx.audioWorklet.addModule(url);
        return new AudioWorkletNode(ctx, 'juno-106', {
          numberOfInputs: 0,
          numberOfOutputs: 1,
          outputChannelCount: [2]
        });
      } catch (e) {
        return null;
      }
    }

    // Runs the worklet source on the main thread inside a fake
    // AudioWorkletGlobalScope, rendered via ScriptProcessorNode. Higher
    // latency than a real worklet, but works everywhere — including
    // browsers that refuse worklet module URLs on file://.
    _makeScriptProcessorNode(ctx, code) {
      var procClass = null;
      var FakeProcessor = function () {
        this.port = { onmessage: null, postMessage: function () {} };
      };
      new Function('sampleRate', 'registerProcessor', 'AudioWorkletProcessor', code)(
        ctx.sampleRate,
        function (name, cls) { procClass = cls; },
        FakeProcessor
      );
      var proc = new procClass();
      var sp = ctx.createScriptProcessor(1024, 0, 2);
      sp.onaudioprocess = function (e) {
        proc.process(null, [[
          e.outputBuffer.getChannelData(0),
          e.outputBuffer.getChannelData(1)
        ]]);
      };
      console.info('Oh-a-synth: AudioWorklet unavailable, using main-thread fallback');
      return {
        port: {
          postMessage: function (m) {
            if (proc.port.onmessage) proc.port.onmessage({ data: m });
          }
        },
        connect: function (dest) { sp.connect(dest); }
      };
    }

    _send(msg) {
      if (this.node) this.node.port.postMessage(msg);
      else if (this._pending.length < 256) this._pending.push(msg);
    }

    _patchValues() {
      var v = {};
      for (var k in Juno.PARAMS) v[k] = this.patch[k];
      return v;
    }

    setParam(id, value) {
      if (!(id in Juno.PARAMS)) return;
      this.patch[id] = value;
      this._send({ type: 'param', id: id, value: value });
      this._emit('param', id, value);
    }

    getPatch() {
      return Object.assign({}, this.patch);
    }

    loadPatch(p) {
      var fresh = Juno.defaultPatch();
      for (var k in fresh) if (k in p) fresh[k] = p[k];
      fresh.name = p.name || 'UNTITLED';
      this.patch = fresh;
      this._send({ type: 'patch', values: this._patchValues() });
      this._emit('patch', this.getPatch());
    }

    noteOn(note, velocity) {
      note = note | 0;
      if (note < 0 || note > 127) return;
      this.held.add(note);
      this.sustained.delete(note);
      this._send({ type: 'noteOn', note: note, velocity: velocity == null ? 0.8 : velocity });
      this._emit('note', note, true);
    }

    noteOff(note) {
      note = note | 0;
      this.held.delete(note);
      if (this.sustainOn) { this.sustained.add(note); return; }
      this._send({ type: 'noteOff', note: note });
      this._emit('note', note, false);
    }

    setSustain(on) {
      this.sustainOn = !!on;
      if (!on) {
        var self = this;
        this.sustained.forEach(function (n) {
          if (!self.held.has(n)) {
            self._send({ type: 'noteOff', note: n });
            self._emit('note', n, false);
          }
        });
        this.sustained.clear();
      }
    }

    pitchBend(v) { this._send({ type: 'bend', value: v }); this._emit('bend', v); }
    modWheel(v) { this._send({ type: 'mod', value: v }); this._emit('mod', v); }
    lfoTrig(on) { this._send({ type: 'lfoTrig', value: !!on }); }

    allNotesOff() {
      this.held.clear();
      this.sustained.clear();
      this._send({ type: 'allOff' });
      this._emit('alloff');
    }
  };
})();
