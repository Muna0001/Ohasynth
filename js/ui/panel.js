/*
 * Panel UI — builds the Juno-106 front panel from the parameter schema
 * and keeps controls in sync with the engine (patch loads, MIDI, etc).
 */
(function () {
  'use strict';
  window.Juno = window.Juno || {};

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
    var wrap = el('div', 'jn-sl' + (opts.small ? ' jn-sl-small' : ''));
    var track = el('div', 'jn-sl-track', wrap);
    var fill = el('div', 'jn-sl-fill', track);
    var thumb = el('div', 'jn-sl-thumb', track);
    var lab = el('div', 'jn-sl-label', wrap);
    lab.textContent = label;
    wrap.title = label;

    var value = 0;

    function render() {
      var pct = (1 - value) * 100;
      thumb.style.top = pct + '%';
      fill.style.height = (100 - pct) + '%';
    }
    function set(v, silent) {
      value = Math.max(0, Math.min(1, v));
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
      set(Juno.PARAMS[id] ? Juno.PARAMS[id].def : 0);
    });
    // fine adjust with mouse wheel
    track.addEventListener('wheel', function (ev) {
      ev.preventDefault();
      set(value - Math.sign(ev.deltaY) * 0.02);
    }, { passive: false });

    controls[id] = { set: function (v) { set(v, true); } };
    set(engine.patch[id] != null ? engine.patch[id] : 0, true);
    return wrap;
  }

  // ---------------------------------------------------------------
  // Segmented switch (also used as 2-position toggle)
  // ---------------------------------------------------------------
  function seg(engine, id, label, options, opts) {
    opts = opts || {};
    var wrap = el('div', 'jn-seg' + (opts.vertical ? ' jn-seg-v' : ''));
    var btnWrap = el('div', 'jn-seg-btns', wrap);
    var btns = options.map(function (opt, i) {
      var b = el('button', 'jn-seg-btn', btnWrap);
      b.type = 'button';
      b.textContent = opt;
      b.addEventListener('click', function () { set(i); });
      return b;
    });
    var lab = el('div', 'jn-sl-label', wrap);
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
    var wrap = el('div', 'jn-chorus');
    var states = [false, false];
    var btns = [0, 1].map(function (i) {
      var b = el('button', 'jn-ch-btn', wrap);
      b.type = 'button';
      b.innerHTML = '<span class="jn-led"></span>' + (i === 0 ? 'I' : 'II');
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
    var sec = el('section', 'jn-section', parent);
    var head = el('div', 'jn-sec-head', sec);
    head.textContent = name;
    if (accent) head.style.setProperty('--accent', accent);
    var body = el('div', 'jn-sec-body', sec);
    return body;
  }

  // ---------------------------------------------------------------
  // Bender (springs back to center)
  // ---------------------------------------------------------------
  function bender(engine) {
    var wrap = el('div', 'jn-bender');
    var track = el('div', 'jn-bender-track', wrap);
    var lever = el('div', 'jn-bender-lever', track);
    var lab = el('div', 'jn-sl-label', wrap);
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
    var b = el('button', 'jn-trig-btn');
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
  Juno.buildPanel = function (engine, root) {
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

  Juno.buildBender = function (engine, root) {
    var box = el('div', 'jn-bend-box', root);
    var levers = el('div', 'jn-bend-levers', box);
    levers.appendChild(bender(engine));
    levers.appendChild(lfoTrigButton(engine));
    var minis = el('div', 'jn-bend-minis', box);
    minis.appendChild(vslider(engine, 'bendDco', 'DCO', { small: true }));
    minis.appendChild(vslider(engine, 'bendVcf', 'VCF', { small: true }));
  };
})();
