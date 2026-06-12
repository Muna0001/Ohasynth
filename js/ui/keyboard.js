/*
 * On-screen keyboard (C2..C6) + computer-keyboard mapping.
 *
 * Computer keys: a w s e d f t g y h u j k o l p ;  starting at C4,
 * shifted with Z (octave down) / X (octave up).
 */
(function () {
  'use strict';
  window.Juno = window.Juno || {};

  var LOW = 36, HIGH = 84; // C2..C6
  var BLACK = { 1: true, 3: true, 6: true, 8: true, 10: true };

  var KEYMAP = {
    KeyA: 0, KeyW: 1, KeyS: 2, KeyE: 3, KeyD: 4, KeyF: 5, KeyT: 6,
    KeyG: 7, KeyY: 8, KeyH: 9, KeyU: 10, KeyJ: 11, KeyK: 12, KeyO: 13,
    KeyL: 14, KeyP: 15, Semicolon: 16, Quote: 17
  };

  Juno.buildKeyboard = function (engine, root) {
    var keys = {}; // note -> element
    var kb = document.createElement('div');
    kb.className = 'jn-kbd';
    root.appendChild(kb);

    var whiteCount = 0;
    for (var n = LOW; n <= HIGH; n++) if (!BLACK[n % 12]) whiteCount++;
    var whiteW = 100 / whiteCount;

    var wIdx = 0;
    for (n = LOW; n <= HIGH; n++) {
      var isBlack = !!BLACK[n % 12];
      var k = document.createElement('div');
      k.className = isBlack ? 'jn-key jn-key-b' : 'jn-key jn-key-w';
      k.dataset.note = n;
      if (isBlack) {
        k.style.left = (wIdx * whiteW - whiteW * 0.3) + '%';
        k.style.width = (whiteW * 0.6) + '%';
      } else {
        k.style.left = (wIdx * whiteW) + '%';
        k.style.width = whiteW + '%';
        if (n % 12 === 0) {
          var lab = document.createElement('span');
          lab.className = 'jn-key-lab';
          lab.textContent = 'C' + (n / 12 - 1);
          k.appendChild(lab);
        }
        wIdx++;
      }
      kb.appendChild(k);
      keys[n] = k;
    }

    function setLit(note, on) {
      var k = keys[note];
      if (k) k.classList.toggle('held', on);
    }
    engine.on('note', setLit);
    engine.on('alloff', function () {
      for (var n in keys) keys[n].classList.remove('held');
    });

    // --- mouse / touch with glide ---------------------------------
    var pointerNotes = {}; // pointerId -> note

    function noteFromEvent(ev) {
      var t = document.elementFromPoint(ev.clientX, ev.clientY);
      if (t && t.classList && t.classList.contains('jn-key-lab')) t = t.parentElement;
      if (t && t.dataset && t.dataset.note) return +t.dataset.note;
      return null;
    }
    kb.addEventListener('pointerdown', function (ev) {
      ev.preventDefault();
      engine.start();
      kb.setPointerCapture(ev.pointerId);
      var n = noteFromEvent(ev);
      if (n != null) { pointerNotes[ev.pointerId] = n; engine.noteOn(n, 0.8); }
    });
    kb.addEventListener('pointermove', function (ev) {
      if (!(ev.pointerId in pointerNotes)) return;
      var n = noteFromEvent(ev);
      var cur = pointerNotes[ev.pointerId];
      if (n !== cur) {
        if (cur != null) engine.noteOff(cur);
        pointerNotes[ev.pointerId] = n;
        if (n != null) engine.noteOn(n, 0.8);
      }
    });
    function release(ev) {
      var cur = pointerNotes[ev.pointerId];
      if (cur != null) engine.noteOff(cur);
      delete pointerNotes[ev.pointerId];
    }
    kb.addEventListener('pointerup', release);
    kb.addEventListener('pointercancel', release);

    // --- computer keyboard -----------------------------------------
    var octShift = 0;
    var down = {}; // code -> note

    function isTyping() {
      var a = document.activeElement;
      return a && (a.tagName === 'INPUT' || a.tagName === 'SELECT' || a.tagName === 'TEXTAREA');
    }

    window.addEventListener('keydown', function (ev) {
      if (ev.repeat || ev.metaKey || ev.ctrlKey || ev.altKey || isTyping()) return;
      if (ev.code === 'KeyZ') { octShift = Math.max(octShift - 1, -2); return; }
      if (ev.code === 'KeyX') { octShift = Math.min(octShift + 1, 2); return; }
      var off = KEYMAP[ev.code];
      if (off == null) return;
      ev.preventDefault();
      engine.start();
      var note = 60 + octShift * 12 + off;
      if (down[ev.code] != null) return;
      down[ev.code] = note;
      engine.noteOn(note, 0.8);
    });
    window.addEventListener('keyup', function (ev) {
      var note = down[ev.code];
      if (note == null) return;
      delete down[ev.code];
      engine.noteOff(note);
    });
    window.addEventListener('blur', function () { engine.allNotesOff(); down = {}; });

    return { setLit: setLit };
  };
})();
