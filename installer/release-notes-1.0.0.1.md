# ForTran 1.0.0.1

A polyphonic additive/formant synthesiser for macOS, as a **VST3** and an
**Audio Unit**. Originally a Cakewalk DXi written in 2003; this is the port.

Ten sine partials, each with its own level, phase, invert and ADSR; ten
formant envelopes over them; a sweepable filter with four switchable shapes
(low pass, high pass, comb, grass); amplitude and filter envelopes; vibrato,
tremolo and a per-harmonic LFO; sixteen voices.

## Installing

Download `ForTran-1.0.0.1.pkg` and open it. The installer offers the two
formats separately:

    /Library/Audio/Plug-Ins/VST3/ForTran.vst3
    /Library/Audio/Plug-Ins/Components/ForTran.component

Quit your DAW first — a running host holds the old copy open. Signed with a
Developer ID and notarised by Apple, so it installs without any
Privacy & Security detour.

Requires macOS 10.13 or later.

## Two things that will look like bugs and are not

* **The Level knob starts 20 dB down.** The 2003 original ran hot enough to
  clip a dense chord. The top of that knob's travel is exactly the original's
  level, so nothing is lost — it is one turn away.
* **The pitch wheel covers a whole octave** either way, not two semitones.
  That is the DXi's own law, kept deliberately.

## Known gaps

* **Program change does not select the waveform.** In the DXi the MIDI patch
  number chose sine, square or saw; this port is sine partials only.
* **The output ladder's corner moves with the sample rate.** Its coefficients
  are constants the original never scaled, so the same patch is more than an
  octave brighter at 96 kHz than at 44.1 kHz. Reproduced rather than fixed;
  the Ladder switch takes it out.

Everything deliberate about the port — every deviation from the original and
why — is in `PORTING-NOTES.md`.

## Uninstalling

A `.pkg` never will, so:

```sh
sudo rm -rf /Library/Audio/Plug-Ins/VST3/ForTran.vst3
sudo rm -rf /Library/Audio/Plug-Ins/Components/ForTran.component
```

## Checksum

```
<paste the output of: shasum -a 256 installer/ForTran-1.0.0.1.pkg>
```

---

Copyright 2026 A. E. Cobley. CC BY-SA 4.0.
VST is a trademark of Steinberg Media Technologies GmbH.
