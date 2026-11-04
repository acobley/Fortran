# ForTran 1.3.1.0

A polyphonic additive/formant synthesiser for macOS, as a **VST3** and an
**Audio Unit**. Originally a Cakewalk DXi written in 2003; this is the port.

## What changed since 1.3.0.0

- **High notes no longer alias.** Like the 2003 DXi, ForTran summed all ten
  partials whatever the note, so on high notes the upper partials went past
  the Nyquist frequency and folded back as tones that are not in the
  harmonic series. At 48 kHz that started at D♯7; at C8 the fold was only
  5.9 dB under the fundamental, and with the ladder in, at G8, it was louder
  than the fundamental. Each partial now **fades out over the last 5 % below
  Nyquist and stops above it**. That is judged on every sample, so a pitch
  bend across the line fades rather than clicks. The worst fold is now
  −150 dB or lower. The live partial bars show the fade.
- **Below about C7 nothing changes.** Every fade there is exactly 1 and is
  never applied: renders of C4, C6 and C7, with the ladder, a harmonic LFO
  and a bend, are byte-identical to 1.3.0.0.
- **Output Level at the bottom is exactly silent.** Its smoother now lands on
  zero instead of hanging just above it.
- **Flush-to-zero on the audio thread.** Each processing block now runs with
  the CPU's flush-to-zero mode on, and puts the host's setting back
  afterwards, so a signal decaying towards silence can never slow the
  plug-in down.

Parameters, presets and saved projects are exactly as in 1.3.0.0. Only notes
high enough to have been folding sound different, and they now sound as they
should.

## Factory presets

Thirteen patches, installed into `/Library/Audio/Presets/AE Cobley/ForTran`
and offered as their own choice in the installer. One folder serves both
formats - the VST3 and the Audio Unit each read that path - and both carry
identical state, so a patch sounds the same in Logic and in Cubase.

Three of them were saved before pan and portamento existed and load with
those two at their defaults: centred, and no glide.

## What it is

Ten sine partials, each with its own level, phase, invert and ADSR; ten
formant envelopes over them; a sweepable filter with four switchable shapes
(low pass, high pass, comb, grass); amplitude and filter envelopes; vibrato,
tremolo and a per-harmonic LFO; sixteen voices.

## Installing

Download `ForTran-1.3.1.0.pkg` and open it. The installer offers each part
separately:

    /Library/Audio/Plug-Ins/VST3/ForTran.vst3
    /Library/Audio/Plug-Ins/Components/ForTran.component
    /Library/Audio/Presets/AE Cobley/ForTran

Quit your DAW first - a running host holds the old copy open. Signed with a
Developer ID and notarised by Apple, so it installs without any
Privacy & Security detour. Installing over an earlier version replaces it; your projects
and their settings are untouched.

Requires macOS 10.13 or later.

## Two things that will look like bugs and are not

* **The Level knob starts 20 dB down.** The 2003 original ran hot enough to
  clip a dense chord. The top of that knob's travel is exactly the original's
  level, so nothing is lost - it is one turn away.
* **The pitch wheel covers a whole octave** either way, not two semitones.
  That is the DXi's own law, kept deliberately.

## Known gaps

* **Program change does not select the waveform.** In the DXi the MIDI patch
  number chose sine, square or saw; this port is sine partials only.
* **The output ladder's corner moves with the sample rate.** Its coefficients
  are constants the original never scaled, so the same patch is more than an
  octave brighter at 96 kHz than at 44.1 kHz. Reproduced rather than fixed;
  the Ladder switch takes it out.
* **Glide time is latched at note-on**, like everything else a voice reads, so
  moving the knob does not retime a glide already under way.

Everything deliberate about the port - every deviation from the original and
why - is in `PORTING-NOTES.md`.

## Uninstalling

A `.pkg` never will, so:

```sh
sudo rm -rf /Library/Audio/Plug-Ins/VST3/ForTran.vst3
sudo rm -rf /Library/Audio/Plug-Ins/Components/ForTran.component
sudo rm -rf "/Library/Audio/Presets/AE Cobley/ForTran"
sudo rm -rf "/Library/Audio/Presets/A. E. Cobley/ForTran"   # 1.2.0.0 and earlier
```

The last two lines take the factory presets with them. Anything you saved yourself
lives in your own `~/Library/Audio/Presets` and is untouched.

## Checksum

```
SHA-256: <paste from: shasum -a 256 installer/ForTran-1.3.1.0.pkg>
```

---

Copyright 2026 A. E. Cobley. CC BY-SA 4.0.
VST is a trademark of Steinberg Media Technologies GmbH.
