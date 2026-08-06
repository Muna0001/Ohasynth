# Oh-a-synth

A 6-voice emulation of the Roland Juno-106, shipped as **three parts** that
share one DSP design:

| Part | What it is | Where it lives |
|---|---|---|
| **1. Standalone Mac app** | Double-clickable native app — no DAW, no browser | `plugin/` → `./install-app.sh` |
| **2. Web app** | Runs in any browser, zero build steps | repo root (`index.html`) |
| **3. DAW plugin** | AU for Logic + VST3 for everything else | `plugin/` |

Parts 1 and 3 are the same C++ codebase (one CMake build emits the app, the
AU, and the VST3). Part 2 is the JavaScript implementation. The two DSP cores
are line-for-line equivalents — see "Keeping the two DSP cores in sync".

---

## Part 1 — Standalone Mac app

The synth as a normal Mac application: it appears in Launchpad, Spotlight, and
the Dock, and plays with the on-screen keyboard or any attached MIDI keyboard.

Build it (see "Building the C++ parts" below), then:

```sh
./install-app.sh
```

That installs to `/Applications/Oh-a-synth.app`, clears the quarantine flag,
and refreshes Launch Services so the icon and Spotlight entry appear
immediately. Launch with `open -a Oh-a-synth` or from Launchpad.

The window uses the normal macOS title bar, so close/minimise/zoom are the
usual traffic lights at the top left.

Audio and MIDI devices are chosen from **MENU → Audio/MIDI Settings…** in the
panel header. If a newly connected MIDI keyboard isn't responding, enable it
there under "Active MIDI inputs". The current patch is saved automatically and
restored on next launch.

### Patches (MENU button)

Patch management lives behind the **MENU** button next to the patch selector,
so it stays out of the way while playing:

| Item | What it does |
|---|---|
| New Patch | Everything back to defaults (INIT) |
| Save Patch… | Names the current sound and stores it as a user patch |
| Add / Remove Favourite ♥ | Hearts a patch; favourites get their own section at the top of the selector |
| Export Patch… | Writes a `.ohasynth.json` file anywhere you like |
| Import Patch… | Reads one back in and adds it to your user patches |
| Delete Patch | Removes the current user patch (disabled for factory presets) |
| Show Patch Folder | Reveals the folder in Finder |

Storage, shared by the app and the plugin so patches appear in both:

```
~/Library/Application Support/Oh-a-synth/Patches/<name>.ohasynth.json
~/Library/Application Support/Oh-a-synth/favourites.txt
```

**Patches move between the browser and the native builds.** The export format
is the same flat JSON the web app writes (`paramID: value`, plus `name`), so a
patch saved in the app imports in the browser and vice versa. Anything a patch
file doesn't name falls back to its default, so older patches saved before a
parameter existed still load.

---

## Part 2 — Web app

- **Easiest:** open `index.html` directly in Chrome/Edge/Firefox. The DSP core
  is loaded into the AudioWorklet via a blob URL specifically so this works
  from `file://`; if a browser refuses that, the engine falls back to a
  main-thread renderer so it still makes sound.
- **Or:** `node serve.js` and open <http://localhost:8493> (any static server
  works).
- **Or:** the hosted build at <https://muna0001.github.io/Ohasynth/>, deployed
  from `main` by `.github/workflows/static.yml`.

Audio starts on your first click or keypress (browser autoplay policy). Play
with the mouse, the computer keyboard (`A W S E D F T G Y H U J K O L P ;`,
octave with `Z`/`X`), or a MIDI keyboard.

Web MIDI needs Chrome, Edge, or Firefox — **Safari does not support it**. The
MIDI status in the header names the specific problem when a keyboard isn't
found, and clicking it re-scans. Incoming MIDI beat clock is used to sync the
arpeggiator (see below).

Files:

```
index.html
css/ohasynth.css           panel styling
js/engine/worklet.js   the entire DSP core (runs inside an AudioWorklet)
js/engine/engine.js    Oha.Engine — headless API (notes, params, patches)
js/engine/presets.js   9 factory patches
js/ui/panel.js         panel controls built from the param schema
js/ui/keyboard.js      on-screen + computer keyboard
js/ui/midi.js          Web MIDI (notes, velocity, bend, CC1, sustain)
js/main.js             boot + patch save/load/export/import
```

The engine is fully headless: `new Oha.Engine()`, `await engine.start()`,
then `noteOn/noteOff/setParam/loadPatch` — no DOM required. The whole signal
path (6 voices, LFO, envelopes, filter, chorus) runs per-sample inside one
AudioWorklet processor, so parameter changes are smoothed sample-accurately
(no zipper noise) and there is no main-thread jitter in the audio.

---

## Part 3 — DAW plugin (Logic / any DAW)

All panel parameters are host-automatable and saved with the DAW project; the
9 factory presets are exposed as plugin programs and from the patch selector
in the editor. Every instance is independent, so you can run it on as many
tracks as you like. The MENU button works here too and shares the same patch
folder as the standalone app — only "Audio/MIDI Settings…" is app-only, since
the DAW owns the audio device.

Build it (below), then in Logic: rescan via Logic Pro → Settings → Plug-in
Manager (or just restart Logic), and insert **AU Instruments → Oh-a-synth →
Oh-a-synth**. Validate manually with `auval -v aumu Oha1 Ohas`.

## Building the C++ parts (app + plugin)

One build produces all three native artifacts. Requires CMake ≥ 3.22 and the
Xcode command-line tools; JUCE 8 is fetched automatically. The app icon comes
from `assets/icon.png` (wired in via JUCE `ICON_BIG`).

```sh
cd plugin
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build --config Release -j8
```

`CMAKE_OSX_ARCHITECTURES` makes a universal binary that runs on both Apple
Silicon and Intel; drop it to build only for the current machine.

The plugins auto-install for the current user; the app needs `./install-app.sh`
(from the repo root):

- Standalone app: `plugin/build/OhASynth_artefacts/Release/Standalone/`
- AU: `~/Library/Audio/Plug-Ins/Components/Oh-a-synth.component`
- VST3: `~/Library/Audio/Plug-Ins/VST3/Oh-a-synth.vst3`

**Troubleshooting:** if a build fails with `'TargetConditionals.h' file not
found`, the Xcode command-line tools were updated and the cached SDK path is
stale — delete `plugin/build` and reconfigure.

These builds are ad-hoc signed, not notarized. On a Mac other than the build
machine, Gatekeeper needs `xattr -dr com.apple.quarantine <path>` (or
right-click → Open) the first time. Proper distribution needs an Apple
Developer ID.

## Typeface

All three parts use **Archivo** (SIL OFL 1.1, Omnibus-Type) — the same face as
the product site, so the panel reads the same everywhere it appears. It ships
with the app rather than being installed, in two forms:

| Part | Form | Where |
|---|---|---|
| Web app | Both faces inlined as base64 | `css/fonts.css` |
| App + plugin | Three static `.ttf`, compiled in via `juce_add_binary_data` | `assets/fonts/` |

The web app inlines the font instead of linking it because a page opened
straight from disk is an opaque origin and browsers refuse the font fetch —
and opening `index.html` with no server is a supported way to run it. A `data:`
URI needs no fetch at all. Subset to Latin at width 100, both faces come to
~49 KB of CSS.

On the native side `OhaLookAndFeel::getTypefaceForFont` maps plain to Regular,
bold to Bold, and italic to Bold Italic 800 — italic is only the wordmark, and
800 is the weight the site sets on its own brand line. The ♥ in the patch menu
is not in Archivo (nor upstream) and falls through to the system font, as do
the ⊓ / ⊿ waveform glyphs on the web panel.

`tools/make-fonts.sh` regenerates every one of those files from the upstream
variable fonts. It is not part of any build — the outputs are committed, so the
web app still opens with zero build steps and CMake just reads the `.ttf`s. Run
it only to change the typeface itself.

## Keeping the two DSP cores in sync

`js/engine/worklet.js` (JavaScript) and `plugin/Source/OhaDSP.h` (C++) are
deliberate line-for-line equivalents: same polyBLEP oscillators, same ZDF
ladder, same chorus constants, same smoothing time constants, same
arpeggiator. **A change to the sound must be made in both files**, or the web
and native versions drift apart. They are structured identically to make that
mechanical.

The arp division table appears in three places that must agree:
`ARP_DIVISIONS` (worklet.js), `Oha.ARP_DIVISIONS`/`Oha.ARP_LABELS`
(engine.js, for the UI), and `kArpDivisions` (OhaDSP.h).

The one intentional difference: only the plugin reads a host transport
(`setHostTransport`). The web build has no host, so it syncs from MIDI clock
or the manual BPM.

Likewise `js/engine/presets.js` and `plugin/Source/Presets.h` hold the same 9
factory patches — append new ones at the end, so existing plugin program
indices keep pointing at the same sound in already-saved DAW projects — and the
parameter schema in `js/engine/engine.js`
(`Oha.PARAMS`) matches `createParameterLayout()` in
`plugin/Source/PluginProcessor.cpp`.

## Voice path (per voice, as on the hardware)

1. **DCO** — phase accumulator with polyBLEP anti-aliasing. Saw + pulse (PWM
   manual or LFO) + square sub one octave down + shared noise source.
   Range 16'/8'/4'. Tuning is digitally exact — the Juno's signature.
2. **HPF** — 4-position. Position 0 is the famous low-shelf bass *boost*,
   1 is flat, 2/3 progressively cut lows.
3. **VCF** — 24 dB/oct zero-delay-feedback ladder with tanh-bounded feedback.
   Self-oscillates near the top of the resonance range. Cutoff, res, bipolar
   env amount (polarity switch), key follow, LFO amount.
4. **VCA** — env or gate mode, level slider.
5. **ENV** — one ADSR shared by VCF and VCA, RC-style exponential segments.
6. **LFO** — global triangle with rate + delay (fade-in restarts when all
   keys are released, like the hardware). Routes to pitch, PWM, VCF.

Small fixed per-voice offsets on VCF cutoff and VCA gain mimic component
tolerances between the six voice chips; the DCO pitch itself never drifts.

**Chorus** — BBD-style stereo chorus on the summed output: one delay line,
two taps modulated in antiphase by a triangle LFO, wet path band-limited to
~9 kHz like the MN3009 chips. Two latching buttons as on the panel:
I (0.513 Hz, lush), II (0.863 Hz), both = I+II (9.75 Hz fast shallow warble).

## Arpeggiator

Sits at the far left of the panel, ahead of the signal-flow sections.

- **ON** — while on, held keys feed the arp instead of sounding together.
- **MODE** — UP, UP & DOWN, DOWN. Up & down does not repeat the endpoints,
  so a three-note chord cycles `1 2 3 2`.
- **RANGE** — 1, 2, or 3 octaves; the chord repeats transposed up.
- **RATE** — free-running 0.5–20 Hz, or a note division when SYNC is on.
- **SYNC** — lock to an external tempo (below).
- **HOLD** — latch: the pattern keeps cycling after you let go, and the first
  key pressed after releasing everything starts a new chord.
- **BPM** — the tempo used when nothing external is running. When something
  is, this dims and `EXT <tempo>` appears beside it.

Each step sounds for half its length, so the pattern is detached rather than
legato. Notes are triggered through the same 6-voice allocator as the
keyboard, so a fast arp with a long release still steals voices normally.

**Divisions.** The RATE slider snaps to 12 stops when synced, slowest at the
bottom: `1/1, 1/2., 1/2, 1/4., 1/4, 1/8., 1/8, 1/16., 1/16, 1/32., 1/32,
1/64` — standard values each preceded by its dotted variant.

**Where the tempo comes from**, in priority order:

1. **Host transport** (plugin only) — the DAW's tempo *and* playhead, so the
   pattern lands on the beat rather than just running at the right speed.
2. **MIDI beat clock** — 24-ppqn clock from another app or device, which is
   how the standalone app and the web app sync. START/CONTINUE resets the
   pattern to the top; the tempo is measured from tick spacing and realigned
   every beat so it cannot drift. If clock stops arriving, it falls back
   automatically after ~0.5 s.
3. **The BPM field** — manual tempo, used when neither of the above applies.

A synced arp deliberately does *not* restart when you press a key; it stays
on the external grid. A free-running one starts immediately on the first key.

## What to listen for, stage by stage

1. **Core voice** — play STRINGS 1 with chorus off (toggle button I): solid
   pitch, smooth filter sweeps with no stair-stepping, snappy attack on
   FAT BASS.
2. **Modulation** — DCO LFO for vibrato (delay slider postpones onset);
   PWM SRC = LFO with the PWM slider up for the classic moving-pulse sound;
   sub slider fattens everything; HPF position 0 vs 3 on a bass patch.
3. **Polyphony** — hold 6+ notes: oldest voices are stolen, releasing notes
   are preferred for stealing. No clicks on steal.
4. **Chorus** — the headline effect. Toggle I on a held pad: mono → wide
   stereo shimmer. II is slightly faster; I+II is a fast leslie-ish warble
   (LESLIE ORGAN preset).
5. **MIDI & patches** — bend lever springs back (depth set by the DCO mini
   slider, VCF bend optional), mod wheel adds vibrato, sustain pedal works.
   Save/Delete/Export/Import in the header; user patches persist in
   localStorage.
6. **Self-oscillation** — turn all waveforms off, resonance to max, play a
   key: the filter sings a sine, tracking the keyboard via KYBD.

## Where the emulation diverges from the hardware

- **Filter chip** — the IR3109 OTA cascade is modeled as a ZDF ladder with a
  tanh nonlinearity. Self-oscillation and resonance feel are close, but the
  exact saturation character of the OTA stages differs.
- **BBD artifacts** — real bucket brigades add clock noise, companding and
  hiss. The chorus here models the delay/modulation/bandwidth, not the dirt,
  so it is cleaner than a real 106 (arguably a feature).
- **Velocity** — the real Juno-106 ignores velocity. MIDI velocity is mapped
  gently to VCA level via the VEL slider (set it to 0 for authentic behavior).
- **Envelope curves** — RC-style exponentials matched by feel, not traced
  from the hardware curve shapes.
- **Latency** — set by the browser (`latencyHint: 'interactive'`), typically
  3–12 ms; a hardware synth is effectively 0. (The plugin build runs at the
  host's buffer size instead.)
