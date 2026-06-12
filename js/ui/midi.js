/*
 * Web MIDI: note on/off + velocity, pitch bend, mod wheel (CC1),
 * sustain pedal (CC64). Listens on all inputs / all channels.
 */
(function () {
  'use strict';
  window.Juno = window.Juno || {};

  Juno.initMIDI = function (engine, statusEl) {
    function setStatus(txt, ok) {
      if (!statusEl) return;
      statusEl.textContent = txt;
      statusEl.classList.toggle('ok', !!ok);
    }

    if (!navigator.requestMIDIAccess) {
      setStatus('MIDI: unavailable', false);
      return;
    }

    navigator.requestMIDIAccess({ sysex: false }).then(function (access) {
      function refresh() {
        var names = [];
        access.inputs.forEach(function (inp) {
          names.push(inp.name);
          inp.onmidimessage = onMessage;
        });
        setStatus(names.length ? 'MIDI: ' + names.join(', ') : 'MIDI: no devices', names.length > 0);
      }
      access.onstatechange = refresh;
      refresh();
    }, function () {
      setStatus('MIDI: access denied', false);
    });

    function onMessage(ev) {
      var d = ev.data;
      if (!d || d.length < 2) return;
      var status = d[0] & 0xf0;
      // resume audio on first MIDI input if needed
      engine.start();
      switch (status) {
        case 0x90: // note on (vel 0 == note off)
          if (d[2] > 0) engine.noteOn(d[1], d[2] / 127);
          else engine.noteOff(d[1]);
          break;
        case 0x80:
          engine.noteOff(d[1]);
          break;
        case 0xE0: { // pitch bend, 14-bit
          var v = ((d[2] << 7) | d[1]) - 8192;
          engine.pitchBend(v / 8192);
          break;
        }
        case 0xB0:
          if (d[1] === 1) engine.modWheel(d[2] / 127);
          else if (d[1] === 64) engine.setSustain(d[2] >= 64);
          else if (d[1] === 120 || d[1] === 123) engine.allNotesOff();
          break;
      }
    }
  };
})();
