/*
 * Web MIDI: note on/off + velocity, pitch bend, mod wheel (CC1),
 * sustain pedal (CC64). Listens on all inputs / all channels.
 *
 * The status element doubles as a re-scan/retry button, and the
 * messages say what to do when MIDI can't work (wrong browser,
 * insecure origin, denied permission).
 */
(function () {
  'use strict';
  window.Juno = window.Juno || {};

  Juno.initMIDI = function (engine, statusEl) {
    function setStatus(txt, ok, tooltip) {
      if (!statusEl) return;
      statusEl.textContent = txt;
      statusEl.classList.toggle('ok', !!ok);
      statusEl.title = tooltip || txt;
    }

    if (!navigator.requestMIDIAccess) {
      if (window.isSecureContext === false)
        setStatus('MIDI: needs https or localhost', false,
          'Web MIDI is disabled on insecure http:// pages. Open the app via ' +
          'https, localhost, or directly as a file.');
      else
        setStatus('MIDI: not supported here', false,
          'This browser has no Web MIDI (Safari does not support it). ' +
          'Use Chrome, Edge, or Firefox for MIDI keyboards.');
      return;
    }

    var access = null;

    function refresh() {
      var names = [];
      access.inputs.forEach(function (inp) {
        names.push(inp.name);
        inp.onmidimessage = onMessage;
      });
      if (names.length)
        setStatus('MIDI: ' + names.join(', '), true,
          'Connected MIDI inputs: ' + names.join(', '));
      else
        setStatus('MIDI: no devices — click to re-scan', false,
          'No MIDI inputs found. Connect a keyboard and click here to re-scan.');
    }

    function request() {
      navigator.requestMIDIAccess({ sysex: false }).then(function (a) {
        access = a;
        a.onstatechange = refresh;
        refresh();
      }, function () {
        setStatus('MIDI: permission denied — click to retry', false,
          'The browser blocked MIDI access. Click to ask again, or allow ' +
          'MIDI in the site settings (icon next to the address bar).');
      });
    }

    if (statusEl) {
      statusEl.style.cursor = 'pointer';
      statusEl.addEventListener('click', request);
    }
    request();

    function onMessage(ev) {
      var d = ev.data;
      if (!d || d.length < 2) return;
      var status = d[0] & 0xf0;
      // Try to start audio. MIDI input is not a user gesture, so this can
      // fail under autoplay policy — main.js shows a "click to enable
      // audio" banner in that case.
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
