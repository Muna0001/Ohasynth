/*
 * Juno-106 DSP core — runs inside an AudioWorklet.
 *
 * The whole of Juno.workletMain is stringified and loaded into the
 * AudioWorkletGlobalScope via a blob URL (so the app works from file://
 * with zero build steps). It must therefore be fully self-contained:
 * no references to anything outside this function.
 *
 * Signal path per voice (matches the hardware):
 *   DCO (polyBLEP saw + pulse w/ PWM + sub square -1 oct + noise)
 *   -> 4-position HPF -> 24 dB/oct ZDF ladder VCF (self-oscillating)
 *   -> VCA (env or gate)
 * Global: single triangle LFO with delay/fade-in, shared ADSR settings,
 * and the BBD chorus (Off / I / II / I+II) on the summed output.
 */
(function () {
  'use strict';
  window.Juno = window.Juno || {};

  Juno.workletMain = function () {
    'use strict';

    var FS = sampleRate;
    var TWO_PI = Math.PI * 2;
    var NUM_VOICES = 6;

    function clamp(x, lo, hi) { return x < lo ? lo : (x > hi ? hi : x); }
    function expMap(v, lo, hi) { return lo * Math.pow(hi / lo, v); }
    // one-pole coefficient from a time constant (seconds)
    function tauCoef(t) { return 1 - Math.exp(-1 / (Math.max(t, 0.00005) * FS)); }
    // one-pole coefficient from a corner frequency (Hz)
    function fcCoef(fc) { return 1 - Math.exp(-TWO_PI * fc / FS); }

    var LOG2_20 = Math.log(20) / Math.LN2;
    var LOG2_22K = Math.log(22000) / Math.LN2;
    var MAX_FC = Math.min(20000, FS * 0.45);

    // ------------------------------------------------------------------
    // Parameter smoother (kills zipper noise)
    // ------------------------------------------------------------------
    function Smoother(v, tSec) {
      this.v = v;
      this.t = v;
      this.c = tauCoef(tSec);
    }
    Smoother.prototype.tick = function () {
      this.v += (this.t - this.v) * this.c;
      return this.v;
    };

    // ------------------------------------------------------------------
    // polyBLEP residual for band-limited edges
    // ------------------------------------------------------------------
    function polyblep(t, dt) {
      if (t < dt) { t = t / dt; return t + t - t * t - 1; }
      if (t > 1 - dt) { t = (t - 1) / dt; return t * t + t + t + 1; }
      return 0;
    }

    // Tiny fixed per-voice component tolerances. The DCO pitch itself is
    // rock solid (that is the Juno character); only VCF cutoff and VCA
    // gain drift slightly between voices, like real OTA chips.
    var V_FC_OFF = [0.016, -0.021, 0.009, -0.014, 0.022, -0.008]; // octaves
    var V_VCA_OFF = [1.0, 0.987, 1.011, 0.992, 1.007, 0.996];

    // ------------------------------------------------------------------
    // Voice
    // ------------------------------------------------------------------
    function Voice(idx) {
      this.idx = idx;
      this.active = false;
      this.note = 60;
      this.velG = 1;
      this.gate = false;
      this.age = 0;
      this.phase = Math.random();
      this.subPhase = Math.random();
      this.env = 0;
      this.stage = 0;           // 0 idle, 1 attack, 2 decay, 3 release
      this.gateLvl = 0;         // smoothed gate for VCA gate mode
      this.s1 = 0; this.s2 = 0; this.s3 = 0; this.s4 = 0; // ladder states
      this.hpY = 0; this.hpX = 0; // HPF state
      this.shelfLp = 0;           // low-shelf state for HPF position 0
    }

    // ------------------------------------------------------------------
    // BBD chorus. Two reads from one delay line, modulated by a triangle
    // LFO in antiphase between L and R — the classic Juno stereo trick.
    // Mode constants follow published analyses of the MN3009 circuit.
    // ------------------------------------------------------------------
    var CHORUS_MODES = [
      { wet: 0.0, rate: 0.500, min: 0.00300, max: 0.00420 }, // off
      { wet: 0.5, rate: 0.513, min: 0.00154, max: 0.00515 }, // I  : slow & lush
      { wet: 0.5, rate: 0.863, min: 0.00154, max: 0.00515 }, // II : faster, deeper feel
      { wet: 0.5, rate: 9.750, min: 0.00322, max: 0.00356 }  // I+II: fast shallow warble
    ];

    function Chorus() {
      this.size = Math.ceil(0.025 * FS);
      this.buf = new Float32Array(this.size);
      this.w = 0;
      this.phase = 0;
      this.rate = CHORUS_MODES[0].rate;
      this.wet = new Smoother(0, 0.05);
      this.center = new Smoother(0.0036, 0.08);
      this.depth = new Smoother(0.0006, 0.08);
      // BBD bandwidth is limited (~10 kHz) and the wet path is AC-coupled
      this.lpCL = 0; this.lpCR = 0;
      this.lpC = fcCoef(9000);
      this.hpL = 0; this.hpLx = 0; this.hpR = 0; this.hpRx = 0;
      this.hpA = 1 - fcCoef(60);
      this.outL = 0; this.outR = 0;
    }
    Chorus.prototype.setMode = function (m) {
      var md = CHORUS_MODES[m] || CHORUS_MODES[0];
      this.rate = md.rate;
      this.wet.t = md.wet;
      this.center.t = (md.min + md.max) * 0.5;
      this.depth.t = (md.max - md.min) * 0.5;
    };
    Chorus.prototype.read = function (delaySec) {
      var d = delaySec * FS;
      if (d < 2) d = 2;
      var rp = this.w - d;
      var i0 = Math.floor(rp);
      var fr = rp - i0;
      var n = this.size;
      var im1 = ((i0 - 1) % n + n) % n;
      var ia = ((i0) % n + n) % n;
      var ib = (ia + 1) % n;
      var ic = (ia + 2) % n;
      var xm1 = this.buf[im1], x0 = this.buf[ia], x1 = this.buf[ib], x2 = this.buf[ic];
      // 4-point Hermite — clean under heavy delay modulation
      var c1 = 0.5 * (x1 - xm1);
      var c2 = xm1 - 2.5 * x0 + 2 * x1 - 0.5 * x2;
      var c3 = 0.5 * (x2 - xm1) + 1.5 * (x0 - x1);
      return ((c3 * fr + c2) * fr + c1) * fr + x0;
    };
    Chorus.prototype.process = function (x) {
      this.buf[this.w] = x;
      this.w = (this.w + 1) % this.size;
      this.phase += this.rate / FS;
      if (this.phase >= 1) this.phase -= 1;
      var tri = this.phase < 0.5 ? this.phase * 4 - 1 : 3 - this.phase * 4;
      var wet = this.wet.tick();
      if (wet < 0.0005) { this.outL = x; this.outR = x; return; }
      var c = this.center.tick();
      var dep = this.depth.tick();
      var wl = this.read(c + dep * tri);
      var wr = this.read(c - dep * tri);
      // BBD bandwidth limit
      this.lpCL += (wl - this.lpCL) * this.lpC;
      this.lpCR += (wr - this.lpCR) * this.lpC;
      // AC-couple the wet path
      this.hpL = this.hpA * (this.hpL + this.lpCL - this.hpLx); this.hpLx = this.lpCL;
      this.hpR = this.hpA * (this.hpR + this.lpCR - this.hpRx); this.hpRx = this.lpCR;
      var dry = 1 - wet * 0.35; // keep loudness roughly constant when engaged
      this.outL = x * dry + this.hpL * wet * 1.25;
      this.outR = x * dry + this.hpR * wet * 1.25;
    };

    // ------------------------------------------------------------------
    // Processor
    // ------------------------------------------------------------------
    class JunoProcessor extends AudioWorkletProcessor {
      constructor() {
        super();
        this.voices = [];
        for (var i = 0; i < NUM_VOICES; i++) this.voices.push(new Voice(i));
        this.ageCounter = 0;
        this.heldNotes = new Set();

        this.P = {}; // raw patch values

        this.sm = {
          pw: new Smoother(0.5, 0.012),
          saw: new Smoother(1, 0.008),
          pulse: new Smoother(0, 0.008),
          sub: new Smoother(0, 0.012),
          noise: new Smoother(0, 0.012),
          cutoff: new Smoother(0.5, 0.018),
          res: new Smoother(0, 0.018),
          envAmt: new Smoother(0, 0.015),
          vcfLfo: new Smoother(0, 0.015),
          keyFlw: new Smoother(0, 0.02),
          vcaLvl: new Smoother(0.8, 0.012),
          volume: new Smoother(0.75, 0.015),
          dcoLfo: new Smoother(0, 0.015),
          bend: new Smoother(0, 0.006),
          mod: new Smoother(0, 0.02)
        };

        // derived / recomputed on parameter messages
        this.aC = tauCoef(0.001); this.dC = tauCoef(0.1);
        this.rC = tauCoef(0.1); this.sus = 0.8;
        this.lfoRateHz = 1;
        this.lfoDelaySec = 0; this.lfoFadeSec = 0.05;
        this.hpfPos = 1;
        this.hpA = 1 - fcCoef(245);
        this.shelfC = fcCoef(150);
        this.bendSemis = 2;
        this.velSens = 0.3;
        this.rangeMult = 1;

        this.lfoPhase = 0;
        this.lfoTime = 100; // long-held => fade complete
        this.lfoTrigHeld = false;

        this.noiseVal = 0;
        this.gateC = tauCoef(0.0025);

        this.chorus = new Chorus();
        this.dcL = 0; this.dcLx = 0; this.dcR = 0; this.dcRx = 0;
        this.dcA = 1 - fcCoef(12);

        var self = this;
        this.port.onmessage = function (e) { self.onMsg(e.data); };
      }

      onMsg(m) {
        switch (m.type) {
          case 'patch':
            for (var k in m.values) this.setParam(k, m.values[k], true);
            this.applyAll();
            break;
          case 'param':
            this.setParam(m.id, m.value, false);
            break;
          case 'noteOn': this.noteOn(m.note, m.velocity); break;
          case 'noteOff': this.noteOff(m.note); break;
          case 'bend': this.sm.bend.t = clamp(m.value, -1, 1); break;
          case 'mod': this.sm.mod.t = clamp(m.value, 0, 1); break;
          case 'lfoTrig': this.lfoTrigHeld = !!m.value; break;
          case 'allOff': this.allOff(); break;
        }
      }

      setParam(id, v, deferred) {
        this.P[id] = v;
        if (deferred) return;
        this.applyOne(id);
      }

      applyAll() {
        for (var id in this.P) this.applyOne(id);
      }

      applyOne(id) {
        var P = this.P, sm = this.sm;
        switch (id) {
          case 'dcoPw': sm.pw.t = v01(P.dcoPw); break;
          case 'dcoSaw': sm.saw.t = P.dcoSaw ? 1 : 0; break;
          case 'dcoPulse': sm.pulse.t = P.dcoPulse ? 1 : 0; break;
          case 'dcoSub': sm.sub.t = v01(P.dcoSub); break;
          case 'dcoNoise': sm.noise.t = v01(P.dcoNoise); break;
          case 'dcoLfo': sm.dcoLfo.t = v01(P.dcoLfo); break;
          case 'dcoRange':
            this.rangeMult = P.dcoRange === 0 ? 0.5 : (P.dcoRange === 2 ? 2 : 1);
            break;
          case 'vcfFreq': sm.cutoff.t = v01(P.vcfFreq); break;
          case 'vcfRes': sm.res.t = v01(P.vcfRes); break;
          case 'vcfEnv': sm.envAmt.t = v01(P.vcfEnv); break;
          case 'vcfLfo': sm.vcfLfo.t = v01(P.vcfLfo); break;
          case 'vcfKey': sm.keyFlw.t = v01(P.vcfKey); break;
          case 'vcaLevel': sm.vcaLvl.t = v01(P.vcaLevel); break;
          case 'volume': sm.volume.t = v01(P.volume); break;
          case 'envA':
            // RC attack aimed above 1.0 so it has the snappy analog knee
            this.aC = tauCoef(expMap(v01(P.envA), 0.0015, 3.2) / 1.45);
            break;
          case 'envD': this.dC = tauCoef(expMap(v01(P.envD), 0.0025, 10) / 3.2); break;
          case 'envS': this.sus = v01(P.envS); break;
          case 'envR': this.rC = tauCoef(expMap(v01(P.envR), 0.0025, 10) / 3.2); break;
          case 'lfoRate': this.lfoRateHz = expMap(v01(P.lfoRate), 0.1, 30); break;
          case 'lfoDelay':
            this.lfoDelaySec = Math.pow(v01(P.lfoDelay), 1.5) * 2.5;
            this.lfoFadeSec = 0.05 + this.lfoDelaySec * 0.6;
            break;
          case 'hpf':
            this.hpfPos = P.hpf | 0;
            this.hpA = 1 - fcCoef(this.hpfPos === 3 ? 520 : 245);
            break;
          case 'chorus': this.chorus.setMode(P.chorus | 0); break;
          case 'bendDco': this.bendSemis = v01(P.bendDco) * 12; break;
          case 'velSens': this.velSens = v01(P.velSens); break;
        }
        function v01(x) { return clamp(+x || 0, 0, 1); }
      }

      noteOn(note, vel) {
        note = clamp(note | 0, 0, 127);
        if (this.heldNotes.size === 0) this.lfoTime = 0; // restart LFO delay
        this.heldNotes.add(note);

        var v = this.findVoice(note);
        v.note = note;
        var vn = clamp(vel == null ? 0.8 : vel, 0.01, 1);
        v.velG = 1 - this.velSens * (1 - Math.pow(vn, 1.4));
        v.gate = true;
        v.stage = 1;          // env retriggers from its current level (analog feel)
        v.age = ++this.ageCounter;
        v.active = true;
      }

      noteOff(note) {
        this.heldNotes.delete(note | 0);
        for (var i = 0; i < NUM_VOICES; i++) {
          var v = this.voices[i];
          if (v.active && v.note === note && v.gate) {
            v.gate = false;
            if (v.stage !== 0) v.stage = 3;
          }
        }
      }

      allOff() {
        this.heldNotes.clear();
        for (var i = 0; i < NUM_VOICES; i++) {
          var v = this.voices[i];
          v.gate = false;
          if (v.active) v.stage = 3;
        }
      }

      // Voice allocation: reuse same note, else a free voice, else the
      // oldest releasing voice, else steal the oldest held voice.
      findVoice(note) {
        var i, v, best = null;
        for (i = 0; i < NUM_VOICES; i++) {
          v = this.voices[i];
          if (v.active && v.note === note) return v;
        }
        for (i = 0; i < NUM_VOICES; i++) {
          if (!this.voices[i].active) return this.voices[i];
        }
        for (i = 0; i < NUM_VOICES; i++) {
          v = this.voices[i];
          if (v.stage === 3 && (best === null || v.age < best.age)) best = v;
        }
        if (best) return best;
        for (i = 0; i < NUM_VOICES; i++) {
          v = this.voices[i];
          if (best === null || v.age < best.age) best = v;
        }
        return best;
      }

      process(inputs, outputs) {
        var out = outputs[0];
        var L = out[0];
        var R = out.length > 1 ? out[1] : out[0];
        var n = L.length;

        var P = this.P, sm = this.sm;
        var pwmLfoMode = P.dcoPwmMode === 1;
        var gateMode = P.vcaMode === 1;
        var envSign = P.vcfPol === 1 ? -1 : 1;
        var rangeMult = this.rangeMult;
        var hpfPos = this.hpfPos;
        var hpA = this.hpA;
        var shelfC = this.shelfC;
        var bendVcf = clamp(+P.bendVcf || 0, 0, 1);
        var lfoInc = this.lfoRateHz / FS;
        var dly = this.lfoDelaySec, fadeT = this.lfoFadeSec;
        var aC = this.aC, dC = this.dC, rC = this.rC, sus = this.sus;
        var gateC = this.gateC;
        var holding = this.heldNotes.size > 0;
        var voices = this.voices;
        var chorus = this.chorus;

        for (var i = 0; i < n; i++) {
          // --- global modulators -----------------------------------
          this.lfoPhase += lfoInc;
          if (this.lfoPhase >= 1) this.lfoPhase -= 1;
          var lfoRaw = this.lfoPhase < 0.5 ? this.lfoPhase * 4 - 1 : 3 - this.lfoPhase * 4;
          if (holding) this.lfoTime += 1 / FS;
          var lfoEnv = this.lfoTrigHeld ? 1 :
            clamp((this.lfoTime - dly) / fadeT, 0, 1);
          var lfo = lfoRaw * lfoEnv;

          var bend = sm.bend.tick();
          var mod = sm.mod.tick();
          var pwAmt = sm.pw.tick();
          var sawG = sm.saw.tick();
          var pulseG = sm.pulse.tick();
          var subLvl = sm.sub.tick();
          var noiseLvl = sm.noise.tick();
          var cutoff = sm.cutoff.tick();
          var res = sm.res.tick();
          var envAmt = sm.envAmt.tick();
          var vcfLfoAmt = sm.vcfLfo.tick();
          var keyFlw = sm.keyFlw.tick();
          var vcaLvl = sm.vcaLvl.tick();
          var volume = sm.volume.tick();
          var vibDepth = clamp(sm.dcoLfo.tick() + mod * 0.4, 0, 1.2);

          var pitchSemisMod = lfo * vibDepth * vibDepth * 8 + bend * this.bendSemis;

          // pulse width: MAN = slider sets it; LFO = LFO sweeps from 50%
          // toward narrow, slider sets sweep depth (like the hardware)
          var pw = pwmLfoMode
            ? 0.5 - (0.5 + 0.5 * lfo) * pwAmt * 0.44
            : 0.5 - pwAmt * 0.44;
          if (pw < 0.05) pw = 0.05;

          var baseCutOct = LOG2_20 + cutoff * (LOG2_22K - LOG2_20)
            + lfo * vcfLfoAmt * 3
            + bend * bendVcf * 3;

          var k = res * 4.6; // ~4.0 is the self-oscillation threshold
          var resComp = 1 + res * 0.65;

          // single noise source shared by all voices, like the hardware
          this.noiseVal = Math.random() * 2 - 1;
          var noiseSig = this.noiseVal * noiseLvl * 0.55;

          var acc = 0;

          // --- voices ----------------------------------------------
          for (var vi = 0; vi < NUM_VOICES; vi++) {
            var v = voices[vi];
            if (!v.active) continue;

            // envelope
            if (v.stage === 1) {
              v.env += (1.28 - v.env) * aC;
              if (v.env >= 1) { v.env = 1; v.stage = 2; }
            } else if (v.stage === 2) {
              v.env += (sus - v.env) * dC;
            } else if (v.stage === 3) {
              v.env += (0 - v.env) * rC;
            }
            v.gateLvl += ((v.gate ? 1 : 0) - v.gateLvl) * gateC;

            var amp = gateMode ? v.gateLvl : v.env;
            if (!v.gate && amp < 0.0001) {
              v.active = false; v.stage = 0; v.env = 0;
              v.s1 = v.s2 = v.s3 = v.s4 = 0;
              continue;
            }

            // --- DCO ---
            var semis = (v.note - 60) + pitchSemisMod;
            var f = 261.625565 * Math.pow(2, semis / 12) * rangeMult;
            if (f > FS * 0.45) f = FS * 0.45;
            var dt = f / FS;

            v.phase += dt;
            if (v.phase >= 1) v.phase -= 1;
            var t = v.phase;

            var saw = (2 * t - 1 - polyblep(t, dt)) * sawG;

            var tp = t - pw + 1; if (tp >= 1) tp -= 1;
            var pulse = ((t < pw ? 1 : -1)
              + polyblep(t, dt) - polyblep(tp, dt)) * pulseG;

            var dt2 = dt * 0.5;
            v.subPhase += dt2;
            if (v.subPhase >= 1) v.subPhase -= 1;
            var st = v.subPhase;
            var st2 = st + 0.5; if (st2 >= 1) st2 -= 1;
            var sub = ((st < 0.5 ? 1 : -1)
              + polyblep(st, dt2) - polyblep(st2, dt2)) * subLvl;

            var sig = saw * 0.5 + pulse * 0.5 + sub * 0.6 + noiseSig;

            // --- HPF (4-position; pos 0 is the famous bass boost) ---
            if (hpfPos === 0) {
              v.shelfLp += (sig - v.shelfLp) * shelfC;
              sig = sig + v.shelfLp * 0.55;
            } else if (hpfPos >= 2) {
              v.hpY = hpA * (v.hpY + sig - v.hpX);
              v.hpX = sig;
              sig = v.hpY;
            }

            // --- VCF: zero-delay-feedback 4-pole ladder ---
            var cutOct = baseCutOct
              + (v.note - 60) / 12 * keyFlw * 1.05
              + envSign * envAmt * v.env * 9
              + V_FC_OFF[vi];
            var fc = Math.pow(2, cutOct);
            if (fc < 8) fc = 8; else if (fc > MAX_FC) fc = MAX_FC;

            var g = Math.tan(Math.PI * fc / FS);
            var G = g / (1 + g);
            var G2 = G * G;

            var S = (G2 * G * v.s1 + G2 * v.s2 + G * v.s3 + v.s4) / (1 + g);
            var u = (sig * 0.7 - k * S) / (1 + k * G2 * G2);
            // tanh bounds the feedback loop => stable self-oscillation
            u = Math.tanh(u);

            var w1 = (u - v.s1) * G; var y1 = w1 + v.s1; v.s1 = y1 + w1;
            var w2 = (y1 - v.s2) * G; var y2 = w2 + v.s2; v.s2 = y2 + w2;
            var w3 = (y2 - v.s3) * G; var y3 = w3 + v.s3; v.s3 = y3 + w3;
            var w4 = (y3 - v.s4) * G; var y4 = w4 + v.s4; v.s4 = y4 + w4;

            // --- VCA ---
            acc += y4 * resComp * amp * vcaLvl * v.velG * V_VCA_OFF[vi];
          }

          acc *= 0.62; // headroom for 6 voices

          // --- chorus + output stage --------------------------------
          chorus.process(acc);
          var lOut = Math.tanh(chorus.outL * volume * 1.6);
          var rOut = Math.tanh(chorus.outR * volume * 1.6);

          // final DC blockers
          this.dcL = this.dcA * (this.dcL + lOut - this.dcLx); this.dcLx = lOut;
          this.dcR = this.dcA * (this.dcR + rOut - this.dcRx); this.dcRx = rOut;

          L[i] = this.dcL;
          R[i] = this.dcR;
        }
        return true;
      }
    }

    registerProcessor('juno-106', JunoProcessor);
  };
})();
