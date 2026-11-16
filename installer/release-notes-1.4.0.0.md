# ForTran 1.4.0.0

A polyphonic additive/formant synthesiser for macOS, as a **VST3** and an
**Audio Unit**. Originally a Cakewalk DXi written in 2003; this is the port.

## What changed since 1.3.1.0

- **The harmonic LFOs can free run.** Up to now every note restarted its ten
  harmonic LFOs at zero, so each note wobbled the same way from its first
  sample. A new switch, **HARMONIC LFOS FREE RUN** (a lamp at the right-hand
  end of the MODULATION title row), keeps one set of phases turning between
  notes instead. Every new note joins it wherever it has got to, so the notes
  of a chord rise and fall together, and repeated notes do not repeat the
  same shape. Off is the default and is the old behaviour.
- **Six new factory presets**, all slow-evolving pads and drones that move
  over tens of seconds: *Tidal Drift*, *Glacier*, *Slow Bloom*,
  *Orbital Comb*, *Lunar Choir* and *Deep Field*. All six use free run.

One parameter has been added, appended after all the others, so no existing
control moves. Projects and presets saved with earlier versions load with
free run off and sound exactly as they did.

## Factory presets

Nineteen patches, installed into `/Library/Audio/Presets/AE Cobley/ForTran`
and offered as their own choice in the installer. One folder serves both
formats - the VST3 and the Audio Unit each read that path - and both carry
identical state, so a patch sounds the same in Logic and in Cubase.

The thirteen older patches load with free run off, which is how they were
made. Three of them were also saved before pan and portamento existed and
load with those two at their defaults: centred, and no glide.

## What it is

Ten sine partials, each with its own level, phase, invert and ADSR; ten
formant envelopes over them; a sweepable filter with four switchable shapes
(low pass, high pass, comb, grass); amplitude and filter envelopes; vibrato,
tremolo and a per-harmonic LFO; sixteen voices.

## Installing

Download `ForTran-1.4.0.0.pkg` and open it. The installer offers each part
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
SHA-256: 1935c060ff1b913ba741501b3462dce1db494cfcb247f6f6a9a8a84647d6457f
```

---

Copyright 2026 A. E. Cobley. CC BY-SA 4.0.
VST is a trademark of Steinberg Media Technologies GmbH.
