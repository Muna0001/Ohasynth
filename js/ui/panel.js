/*
 * Panel UI — builds the front panel from the parameter schema
 * and keeps controls in sync with the engine (patch loads, MIDI, etc).
 */
(function () {
  'use strict';
  window.Oha = window.Oha || {};

  var controls = {}; // paramId -> { set(value) }

  function el(tag, cls, parent) {
    var e = document.createElement(tag);
    if (cls) e.className = cls;
    if (parent) parent.appendChild(e);
    return e;
  }

  // ---------------------------------------------------------------
  // Vertical slider (custom — input[type=range] vertical styling is
  // inconsistent across browsers)
  // ---------------------------------------------------------------
  function vslider(engine, id, label, opts) {
    opts = opts || {};
    var wrap = el('div', 'oha-sl' + (opts.small ? ' oha-sl-small' : ''));
    var track = el('div', 'oha-sl-track', wrap);
    var fill = el('div', 'oha-sl-fill', track);
    var thumb = el('div', 'oha-sl-thumb', track);
    var lab = el('div', 'oha-sl-label', wrap);
    lab.textContent = label;
    // optional value readout under the label (used by the arp RATE slider,
    // which shows either a note division or a free-run rate in Hz)
    var read = opts.readout ? el('div', 'oha-sl-read', wrap) : null;
    wrap.title = label;

    var value = 0;

    // Number of detents, or 0 for continuous. A function lets it change at
    // runtime (RATE snaps to note divisions only while SYNC is on).
    function stepCount() {
      var s = typeof opts.steps === 'function' ? opts.steps() : opts.steps;
      return s > 1 ? s : 0;
    }

    function render() {
      var pct = (1 - value) * 100;
      thumb.style.top = pct + '%';
      fill.style.height = (100 - pct) + '%';
      if (read) read.textContent = opts.readout(value);
    }
    function set(v, silent) {
      v = Math.max(0, Math.min(1, v));
      var sc = stepCount();
      if (sc) v = Math.round(v * (sc - 1)) / (sc - 1);
      value = v;
      render();
      if (!silent) engine.setParam(id, value);
    }

    function fromEvent(ev) {
      var r = track.getBoundingClientRect();
      var v = 1 - (ev.clientY - r.top) / r.height;
      set(v);
    }
    track.addEventListener('pointerdown', function (ev) {
      ev.preventDefault();
      track.setPointerCapture(ev.pointerId);
      fromEvent(ev);
      function move(e) { fromEvent(e); }
      function up() {
        track.removeEventListener('pointermove', move);
        track.removeEventListener('pointerup', up);
        track.removeEventListener('pointercancel', up);
      }
      track.addEventListener('pointermove', move);
      track.addEventListener('pointerup', up);
      track.addEventListener('pointercancel', up);
    });
    track.addEventListener('dblclick', function () {
      set(Oha.PARAMS[id] ? Oha.PARAMS[id].def : 0);
    });
    // fine adjust with mouse wheel
    track.addEventListener('wheel', function (ev) {
      ev.preventDefault();
      set(value - Math.sign(ev.deltaY) * 0.02);
    }, { passive: false });

    controls[id] = {
      set: function (v) { set(v, true); },
      refresh: function () { set(value); }   // re-snap / redraw the readout
    };
    set(engine.patch[id] != null ? engine.patch[id] : 0, true);
    return wrap;
  }

  // ---------------------------------------------------------------
  // Latching button with an LED (arp ON / SYNC / HOLD)
  // ---------------------------------------------------------------
  function ledButton(engine, id, label, onChange) {
    var wrap = el('div', 'oha-ledbtn-wrap');
    var b = el('button', 'oha-ledbtn', wrap);
    b.type = 'button';
    b.appendChild(el('span', 'oha-led'));
    var lab = el('div', 'oha-sl-label', wrap);
    lab.textContent = label;

    var value = 0;
    function set(v, silent) {
      value = v ? 1 : 0;
      b.classList.toggle('on', !!value);
      if (!silent) engine.setParam(id, value);
      if (onChange) onChange(value);
    }
    b.addEventListener('click', function () { set(value ? 0 : 1); });
    controls[id] = { set: function (v) { set(v, true); } };
    set(engine.patch[id] ? 1 : 0, true);
    return wrap;
  }

  // Stack two controls into a single panel column, bottom-aligned.
  function stack() {
    var wrap = el('div', 'oha-stack');
    for (var i = 0; i < arguments.length; i++) wrap.appendChild(arguments[i]);
    return wrap;
  }

  // ---------------------------------------------------------------
  // Tempo used when no external clock is running. When one is, this shows
  // the incoming tempo instead so it is obvious what the arp is following.
  // ---------------------------------------------------------------
  function bpmField(engine) {
    var wrap = el('div', 'oha-bpm');
    var ext = el('div', 'oha-bpm-ext', wrap);
    var input = el('input', 'oha-bpm-input', wrap);
    input.type = 'number';
    input.min = 40; input.max = 300; input.step = 1;
    input.spellcheck = false;

    function push() {
      var v = Math.max(40, Math.min(300, Math.round(+input.value || 120)));
      input.value = v;
      engine.setParam('arpBpm', v);
    }
    input.addEventListener('change', push);
    input.addEventListener('blur', push);

    controls.arpBpm = {
      set: function (v) { input.value = Math.round(+v || 120); }
    };
    controls.arpBpm.set(engine.patch.arpBpm);

    engine.on('clock', function (active, bpm) {
      var live = active && bpm > 0;
      ext.textContent = live ? 'EXT ' + bpm.toFixed(1) : '';
      ext.classList.toggle('on', live);
      input.classList.toggle('overridden', live);
      input.title = live
        ? 'Following an external MIDI clock at ' + bpm.toFixed(1) + ' BPM'
        : 'Arp tempo in BPM, used when no external MIDI clock is running';
    });
    input.title = 'Arp tempo in BPM, used when no external MIDI clock is running';
    return wrap;
  }

  // ---------------------------------------------------------------
  // Segmented switch (also used as 2-position toggle)
  // ---------------------------------------------------------------
  function seg(engine, id, label, options, opts) {
    opts = opts || {};
    var wrap = el('div', 'oha-seg' + (opts.vertical ? ' oha-seg-v' : ''));
    var btnWrap = el('div', 'oha-seg-btns', wrap);
    var btns = options.map(function (opt, i) {
      var b = el('button', 'oha-seg-btn', btnWrap);
      b.type = 'button';
      b.textContent = opt;
      b.addEventListener('click', function () { set(i); });
      return b;
    });
    var lab = el('div', 'oha-sl-label', wrap);
    lab.textContent = label;

    var value = 0;
    function render() {
      btns.forEach(function (b, i) { b.classList.toggle('on', i === value); });
    }
    function set(v, silent) {
      value = v | 0;
      render();
      if (!silent) engine.setParam(id, value);
    }
    controls[id] = { set: function (v) { set(v, true); } };
    set(engine.patch[id] | 0, true);
    return wrap;
  }

  // ---------------------------------------------------------------
  // Chorus: two latching buttons like the hardware (both = I+II)
  // ---------------------------------------------------------------
  function chorusButtons(engine) {
    var wrap = el('div', 'oha-chorus');
    var states = [false, false];
    var btns = [0, 1].map(function (i) {
      var b = el('button', 'oha-ch-btn', wrap);
      b.type = 'button';
      b.innerHTML = '<span class="oha-led"></span>' + (i === 0 ? 'I' : 'II');
      b.addEventListener('click', function () {
        states[i] = !states[i];
        push();
      });
      return b;
    });
    function render() {
      btns.forEach(function (b, i) { b.classList.toggle('on', states[i]); });
    }
    function push() {
      render();
      engine.setParam('chorus', (states[0] ? 1 : 0) + (states[1] ? 2 : 0));
    }
    controls.chorus = {
      set: function (v) {
        states[0] = !!(v & 1);
        states[1] = !!(v & 2);
        render();
      }
    };
    controls.chorus.set(engine.patch.chorus | 0);
    return wrap;
  }

  function section(parent, name, accent) {
    var sec = el('section', 'oha-section', parent);
    var head = el('div', 'oha-sec-head', sec);
    head.textContent = name;
    if (accent) head.style.setProperty('--accent', accent);
    var body = el('div', 'oha-sec-body', sec);
    return body;
  }

  // ---------------------------------------------------------------
  // Bender (springs back to center)
  // ---------------------------------------------------------------
  function bender(engine) {
    var wrap = el('div', 'oha-bender');
    var track = el('div', 'oha-bender-track', wrap);
    var lever = el('div', 'oha-bender-lever', track);
    var lab = el('div', 'oha-sl-label', wrap);
    lab.textContent = 'BENDER';

    var value = 0;
    function render() {
      lever.style.left = (50 + value * 42) + '%';
    }
    function set(v) {
      value = Math.max(-1, Math.min(1, v));
      render();
      engine.pitchBend(value);
    }
    track.addEventListener('pointerdown', function (ev) {
      ev.preventDefault();
      track.setPointerCapture(ev.pointerId);
      function fromEvent(e) {
        var r = track.getBoundingClientRect();
        set(((e.clientX - r.left) / r.width) * 2 - 1);
      }
      fromEvent(ev);
      function move(e) { fromEvent(e); }
      function up() {
        track.removeEventListener('pointermove', move);
        track.removeEventListener('pointerup', up);
        track.removeEventListener('pointercancel', up);
        set(0); // spring back
      }
      track.addEventListener('pointermove', move);
      track.addEventListener('pointerup', up);
      track.addEventListener('pointercancel', up);
    });
    render();
    return wrap;
  }

  function lfoTrigButton(engine) {
    var b = el('button', 'oha-trig-btn');
    b.type = 'button';
    b.textContent = 'LFO TRIG';
    function down(ev) { ev.preventDefault(); b.classList.add('on'); engine.lfoTrig(true); }
    function up() { b.classList.remove('on'); engine.lfoTrig(false); }
    b.addEventListener('pointerdown', down);
    b.addEventListener('pointerup', up);
    b.addEventListener('pointerleave', up);
    return b;
  }

  // ---------------------------------------------------------------
  // Panel assembly
  // ---------------------------------------------------------------
  Oha.buildPanel = function (engine, root) {
    // arpeggio sits at the far left, as on the hardware
    var arp = section(root, 'ARPEGGIO', '#4f9ed9');
    arp.appendChild(stack(ledButton(engine, 'arpOn', 'ON'),
                          ledButton(engine, 'arpHold', 'HOLD')));
    arp.appendChild(seg(engine, 'arpMode', 'MODE', ['UP', 'UP&DN', 'DOWN'], { vertical: true }));
    arp.appendChild(seg(engine, 'arpRange', 'RANGE', ['1', '2', '3'], { vertical: true }));
    arp.appendChild(vslider(engine, 'arpRate', 'RATE', {
      // detented on note divisions while synced, continuous Hz otherwise
      steps: function () { return engine.patch.arpSync ? Oha.ARP_DIVISIONS.length : 0; },
      readout: function (v) {
        return engine.patch.arpSync
          ? Oha.ARP_LABELS[Oha.arpDivIndex(v)]
          : (0.5 * Math.pow(40, v)).toFixed(1) + ' Hz';
      }
    }));
    // BPM sits directly above SYNC, since it only matters when synced
    arp.appendChild(stack(bpmField(engine),
                          ledButton(engine, 'arpSync', 'SYNC', function () {
                            if (controls.arpRate) controls.arpRate.refresh();
                          })));

    // left block: volume
    var left = section(root, 'MAIN', '#c8413b');
    left.appendChild(vslider(engine, 'volume', 'VOLUME'));
    left.appendChild(vslider(engine, 'velSens', 'VEL'));

    var lfo = section(root, 'LFO', '#4f9ed9');
    lfo.appendChild(vslider(engine, 'lfoRate', 'RATE'));
    lfo.appendChild(vslider(engine, 'lfoDelay', 'DELAY'));

    var dco = section(root, 'DCO', '#e0a33b');
    dco.appendChild(seg(engine, 'dcoRange', 'RANGE', ["16'", "8'", "4'"], { vertical: true }));
    dco.appendChild(vslider(engine, 'dcoLfo', 'LFO'));
    dco.appendChild(vslider(engine, 'dcoPw', 'PWM'));
    dco.appendChild(seg(engine, 'dcoPwmMode', 'PWM SRC', ['MAN', 'LFO'], { vertical: true }));
    dco.appendChild(seg(engine, 'dcoPulse', '⊓ PULSE', ['OFF', 'ON'], { vertical: true }));
    dco.appendChild(seg(engine, 'dcoSaw', '⊿ SAW', ['OFF', 'ON'], { vertical: true }));
    dco.appendChild(vslider(engine, 'dcoSub', 'SUB'));
    dco.appendChild(vslider(engine, 'dcoNoise', 'NOISE'));

    var hpf = section(root, 'HPF', '#9bba5e');
    hpf.appendChild(seg(engine, 'hpf', 'FREQ', ['0', '1', '2', '3'], { vertical: true }));

    var vcf = section(root, 'VCF', '#c8413b');
    vcf.appendChild(vslider(engine, 'vcfFreq', 'FREQ'));
    vcf.appendChild(vslider(engine, 'vcfRes', 'RES'));
    vcf.appendChild(seg(engine, 'vcfPol', 'ENV POL', ['↑', '↓'], { vertical: true }));
    vcf.appendChild(vslider(engine, 'vcfEnv', 'ENV'));
    vcf.appendChild(vslider(engine, 'vcfLfo', 'LFO'));
    vcf.appendChild(vslider(engine, 'vcfKey', 'KYBD'));

    var vca = section(root, 'VCA', '#4f9ed9');
    vca.appendChild(seg(engine, 'vcaMode', 'MODE', ['ENV', 'GATE'], { vertical: true }));
    vca.appendChild(vslider(engine, 'vcaLevel', 'LEVEL'));

    var env = section(root, 'ENV', '#e0a33b');
    env.appendChild(vslider(engine, 'envA', 'A'));
    env.appendChild(vslider(engine, 'envD', 'D'));
    env.appendChild(vslider(engine, 'envS', 'S'));
    env.appendChild(vslider(engine, 'envR', 'R'));

    var ch = section(root, 'CHORUS', '#9bba5e');
    ch.appendChild(chorusButtons(engine));

    // refresh all controls when a patch loads
    engine.on('patch', function (p) {
      for (var id in controls) {
        if (id in p) controls[id].set(p[id]);
      }
    });

    return controls;
  };

  Oha.buildBender = function (engine, root) {
    var box = el('div', 'oha-bend-box', root);
    var levers = el('div', 'oha-bend-levers', box);
    levers.appendChild(bender(engine));
    levers.appendChild(lfoTrigButton(engine));
    var minis = el('div', 'oha-bend-minis', box);
    minis.appendChild(vslider(engine, 'bendDco', 'DCO', { small: true }));
    minis.appendChild(vslider(engine, 'bendVcf', 'VCF', { small: true }));
  };
})();
