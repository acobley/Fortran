# ForTran

A VST3 and Audio Unit build of **ForTran**, the additive/formant synthesiser
written in 2003 as a Cakewalk DXi (Visual C++ 6, MFC). The port targets macOS,
builds a `.vst3` and a `.component` from one CMake project, and reproduces the
original's DSP sample for sample except where a deviation is deliberate and
recorded.

It is a genuine polyphonic instrument: no audio input, one stereo output,
sixteen voices, 145 parameters.

* `PORTING-NOTES.md` — what the original did, every deviation and why, every
  trap hit. The long one, and the one to read before changing any DSP.
* `PORTING-GUIDE.md` / `PORT-CHECKLIST.md` — the method and its working copy,
  carried over from `../vst3-port-template`.
* `docs/` — two drawings of the signal path, as HTML sources with PNG renders.

---

## The sound

Per sounding note, per sample:

* **ten sine partials** at `k × theta`, k = 1..10, each with its own level,
  phase offset, invert switch and **its own ADSR** — the "formant" envelopes,
  ten of them;
* a **spectral filter** with four independently switchable shapes — low pass
  (off/on), high pass, comb and grass (off/add/mult/max) — swept by a filter
  envelope, applied as a magnitude *and phase* per partial;
* an **amplitude envelope**, a **tremolo** LFO on level and a **vibrato** LFO
  on the phase step;
* a **per-harmonic amplitude LFO**, rate and depth for each of the ten — the
  DXi allocated the state for this and never wrote to it; here it is finished;
* a fixed **Moog ladder** on the way out, switchable, with a level-matched
  bypass.

Then sixteen voices mix to one stereo bus through the output trim and pan.
`docs/signal-path.png` draws all of it; `docs/partial-bank.png` opens up one
partial.

Two things the measurements say and the name does not. The ladder's cutoff and
resonance are hardcoded, its corner is 0.198 × Nyquist, and its resonance is
+0.22 dB — so below about B4 it is a flat +10.6 dB gain stage and nothing else,
and its corner moves with the sample rate. And the DXi's own gain staging is
hot: the **Level** trim defaults to its centre, 20 dB below the original, with
the top of the knob being exactly the 2003 level. Both are in PORTING-NOTES
section 7.

## Using it

The panel is the DXi's, at the true size of its backdrop (840 × 388) rather
than the smaller Windows dialog that clipped it.

* **Harmonics** — ten vertical level sliders, ten phase knobs, ten invert
  toggles. Alt-click a slider to mute it.
* **Formants** — ten selector buttons, but only **one** set of five knobs:
  clicking a button re-points the ADSR knobs, ENV and INV at that formant.
  That is what the DXi did, and it is editor state, not a parameter.
* **Filter** — cutoff, Q and poles sliders, four text steppers for the shapes,
  and a live response panel.
* **Envelopes** — three panels: amplitude, filter, and the selected formant.
* **A green lamp is on, a red one off** — including the formant selector, where
  the selected one is green. That is the DXi's own convention; this port had
  the two bitmaps the wrong way round until it was caught.
* **Values read out** on a single plate that follows the pointer, appearing
  beside whatever is hovered or dragged. The DXi showed no numbers anywhere,
  and the panel has no room for 42 permanent readouts. Envelope times read as
  times, cutoff in hertz, the shape steppers as Off/Add/Mult/Max.
* Knobs: drag vertically, shift for fine, wheel works.

**MIDI.** Note on/off (velocity zero is a note-off), pitch wheel — the DXi's
law is a **whole octave** either way, not two semitones — CC7 channel volume
on a square-root law, and CC10 pan. The **Pan** knob is new and *offsets* CC10
rather than replacing it: centred, CC10 behaves exactly as it did before the
knob existed.

**Not carried over:** the DXi's multi-output pins, the dead "Synth/Xform" mode
switch, the audio-input passthrough (an instrument has no input), and program
change selecting the oscillator waveform — this port is sine partials only.

## Building

macOS, CMake ≥ 3.25, and either Xcode or the command line tools.

```sh
./setup-xcode.sh
```

The first configure clones the VST3 SDK (v3.8.1, ~250 MB) into `external/`, so
it takes a few minutes; later ones reuse it. `--help` lists the options:

| | |
|---|---|
| `--no-open` | configure only, do not open Xcode |
| `--makefiles` | build without Xcode, into `build-make/` |
| `--no-au` | skip the Audio Unit target |
| `--no-validator` | skip Steinberg's validator after each build |
| `--clean` | throw away `build/` first |

To use an SDK you already have: `VST3_SDK_ROOT=/path ./setup-xcode.sh`.

Both bundles are **ad-hoc signed** automatically, which needs no developer
certificate and is enough for macOS to load them, Apple Silicon included. For
a real identity:

```sh
cmake -B build -G Xcode -DPORT_CODE_SIGN_IDENTITY="Developer ID Application: Your Name (TEAMID)"
```

Every build puts `ForTran.vst3` in `~/Library/Audio/Plug-Ins/VST3/` and
`ForTran.component` in `~/Library/Audio/Plug-Ins/Components/`. Both point back
into the build tree — the VST3 as a link, and inside the `.component` a symlink
to the `.vst3` — so **before giving either to anyone, replace them with the
real bundles.**

**If a build fails with *"Bundle does not export the required
'GetPluginFactory' function"***, you added a source file: the Xcode generator
compiled the old file list. A pre-build check catches this and names what
changed. Re-run `./setup-xcode.sh --no-open` and build again.

## The tests

The DSP is written free of SDK types, so it compiles and runs on its own. Each
file in `tests/` carries its build line in its header comment, e.g.

```sh
cd tests
c++ -std=c++17 -O2 -Wall -Wextra -I../source ../source/Adsr.cpp AdsrTests.cpp -o adsr-tests && ./adsr-tests
```

| | |
|---|---|
| `AdsrTests` | the envelope against a transcription of the 2003 class — bit-identical across 432 settings |
| `FilterInfoTests` | the response tables, likewise transcribed |
| `FilterCombineTests` | how the four shapes fold together |
| `VoiceTests` | one note: the ladder, the sweep, the LFO, panning, levels |
| `SynthTests` | the voice pool — stealing, retriggering, 4,000 unbalanced events |
| `ParamTableTests` | the generated parameter table, all three ranges and round-tripping |

`ParamTableTests` needs the SDK headers on the include path; the rest need
nothing.

## Layout

| | |
|---|---|
| `source/` | the plug-in — seven VST3 files plus `Adsr`, `FilterInfo`, `Voice`, `Synth` and the controls |
| `resource/` | artwork, converted from the DXi's `.bmp`s, and `au-info.plist` |
| `tools/` | the scripts that build the background and button artwork |
| `tests/` | standalone DSP tests |
| `docs/` | the two signal-path diagrams |
| `CMakeLists.txt` | the build, from `../vst3-port-template` — only the identity block is ForTran's |

`source/ForTranParams.cpp` is **generated** from the DXi's own `Parameters.h`,
so the ranges and defaults are the original's by construction. Regenerate it
rather than hand-editing.

## Status

Phases 1–5 are done: parameters, DSP, editor. **Phase 6** — the Audio Unit
scheme, `auval`, a Release-build validator run and a real host — is current.
`PORT-CHECKLIST.md` is the live list; CC7 and CC10 are still to be confirmed
in a host.

## Identity — permanent once shipped

Change any of these and every existing session loses the plug-in.

| | |
|---|---|
| VST3 bundle id | `audio.fortran.vst3` |
| AU bundle id | `audio.fortran.audiounit` |
| AU type / subtype / manufacturer | `aumu` / `FTrn` / `AECo` |
| Processor UID | `0x3B4723D5, 0xF76D4616, 0xAA9D9320, 0x43C2D59E` |
| Controller UID | `0x29D6D36A, 0xB7EC46DB, 0xB03377F9, 0x8A8D647D` |

`AECo` is the author and is shared with the other ports; only the subtype
distinguishes them.
