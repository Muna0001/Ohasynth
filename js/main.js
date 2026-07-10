/*
 * Boot: wires engine + panel + keyboard + MIDI + patch management.
 */
(function () {
  'use strict';

  var LS_KEY = 'ohasynth.userPatches.v1';

  function loadUserPatches() {
    try { return JSON.parse(localStorage.getItem(LS_KEY)) || {}; }
    catch (e) { return {}; }
  }
  function saveUserPatches(p) {
    localStorage.setItem(LS_KEY, JSON.stringify(p));
  }

  window.addEventListener('DOMContentLoaded', function () {
    var engine = new Juno.Engine();
    window.junoEngine = engine; // handy for debugging / headless use

    Juno.buildPanel(engine, document.getElementById('panel'));
    Juno.buildBender(engine, document.getElementById('bender'));
    Juno.buildKeyboard(engine, document.getElementById('keyboard'));
    Juno.initMIDI(engine, document.getElementById('midi-status'));

    // ----- patch bar ------------------------------------------------
    var sel = document.getElementById('patch-select');
    var nameInput = document.getElementById('patch-name');
    var userPatches = loadUserPatches();

    function rebuildSelect(selectedName) {
      sel.innerHTML = '';
      var ogF = document.createElement('optgroup');
      ogF.label = 'Factory';
      Juno.PRESETS.forEach(function (p, i) {
        var o = document.createElement('option');
        o.value = 'f:' + i;
        o.textContent = p.name;
        ogF.appendChild(o);
      });
      sel.appendChild(ogF);
      var names = Object.keys(userPatches).sort();
      if (names.length) {
        var ogU = document.createElement('optgroup');
        ogU.label = 'User';
        names.forEach(function (n) {
          var o = document.createElement('option');
          o.value = 'u:' + n;
          o.textContent = n;
          ogU.appendChild(o);
        });
        sel.appendChild(ogU);
      }
      if (selectedName != null) sel.value = selectedName;
    }

    function applySelection() {
      var v = sel.value;
      var p = null;
      if (v.slice(0, 2) === 'f:') p = Juno.PRESETS[+v.slice(2)];
      else if (v.slice(0, 2) === 'u:') p = userPatches[v.slice(2)];
      if (!p) return;
      engine.loadPatch(p);
      nameInput.value = p.name || '';
    }
    sel.addEventListener('change', function () {
      engine.start();
      applySelection();
    });

    document.getElementById('patch-save').addEventListener('click', function () {
      var name = (nameInput.value || '').trim().toUpperCase() || 'UNTITLED';
      var p = engine.getPatch();
      p.name = name;
      userPatches[name] = p;
      saveUserPatches(userPatches);
      rebuildSelect('u:' + name);
    });

    document.getElementById('patch-delete').addEventListener('click', function () {
      var v = sel.value;
      if (v.slice(0, 2) !== 'u:') return;
      delete userPatches[v.slice(2)];
      saveUserPatches(userPatches);
      rebuildSelect('f:0');
      applySelection();
    });

    document.getElementById('patch-export').addEventListener('click', function () {
      var p = engine.getPatch();
      var blob = new Blob([JSON.stringify(p, null, 2)], { type: 'application/json' });
      var a = document.createElement('a');
      a.href = URL.createObjectURL(blob);
      a.download = (p.name || 'patch').toLowerCase().replace(/[^a-z0-9]+/g, '-') + '.ohasynth.json';
      a.click();
      URL.revokeObjectURL(a.href);
    });

    var importInput = document.getElementById('patch-import-file');
    document.getElementById('patch-import').addEventListener('click', function () {
      importInput.click();
    });
    importInput.addEventListener('change', function () {
      var f = importInput.files[0];
      if (!f) return;
      f.text().then(function (txt) {
        var data = JSON.parse(txt);
        // accept a single patch or an array of patches
        var list = Array.isArray(data) ? data : [data];
        var last = null;
        list.forEach(function (p) {
          if (!p || typeof p !== 'object') return;
          var name = (p.name || 'IMPORTED').toUpperCase();
          p.name = name;
          userPatches[name] = p;
          last = name;
        });
        if (last) {
          saveUserPatches(userPatches);
          rebuildSelect('u:' + last);
          applySelection();
        }
      }).catch(function () { alert('Could not read patch file.'); });
      importInput.value = '';
    });

    // ----- audio autostart ------------------------------------------
    // Browsers only allow audio after a user gesture, so power on with
    // the first interaction of any kind. Note/param events that arrive
    // while starting are queued by the engine and flushed when ready.
    function powerOn() { engine.start(); }
    window.addEventListener('pointerdown', powerOn, { capture: true });
    window.addEventListener('keydown', powerOn, { capture: true });
    engine.on('started', function () {
      window.removeEventListener('pointerdown', powerOn, { capture: true });
      window.removeEventListener('keydown', powerOn, { capture: true });
    });

    // MIDI input is not a user gesture, so playing an external keyboard
    // before touching the page leaves audio suspended — the synth hears
    // the notes but cannot sound. Show a banner explaining the one click
    // it takes to unblock.
    var audioHint = document.createElement('div');
    audioHint.id = 'audio-hint';
    audioHint.textContent = '🔊 Audio is blocked by the browser — click anywhere to enable sound';
    document.body.appendChild(audioHint);
    engine.on('note', function () {
      setTimeout(function () {
        if (!engine.running) audioHint.classList.add('show');
      }, 250);
    });
    engine.on('started', function () { audioHint.classList.remove('show'); });

    // load the first factory preset so it sounds great immediately
    rebuildSelect('f:0');
    applySelection();
  });
})();
