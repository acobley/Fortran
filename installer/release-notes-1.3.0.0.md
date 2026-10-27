# ForTran 1.3.0.0

A polyphonic additive/formant synthesiser for macOS, as a **VST3** and an
**Audio Unit**. Originally a Cakewalk DXi written in 2003; this is the port.

## What changed since 1.2.1.0

**A new panel.** The window is now a black instrument panel, 1200 × 640:
knurled knobs with printed scales, rotary switches, red jewel lamps and small
colour screens, lettered throughout in a 70s sci-fi face, with a light worn
finish. Nothing in the sound or the parameters changed, so **projects and
presets from earlier versions open as they were**.

- **Every partial at once.** The partial bank shows all ten partials' level,
  phase, invert and envelope switch together. Click a numbered ring (or
  ‹ ›) to put a partial's envelope and harmonic LFO in the PARTIAL box;
  **Copy n to all** copies that box to the other nine.
- **Values on the panel.** Each knob's value is printed under it. Hovering
  still shows the full name and value; Alt-click or double-click a knob to
  return it to its default.
- **Live screens.** While a note sounds, the partial bars show what each
  partial is really putting out, with the output waveform behind them; the
  filter screen's dots slide along the response as the filter envelope
  sweeps; the amp and partial envelopes carry a playhead; a voice meter in
  the header counts the sounding notes.
- **The harmonic LFOs are on the panel.** The pop-up is gone: drag one of the
  ten rings up or down for depth, sideways for rate (the inner arc and the
  figure under the ring show the rate; a dot runs round at that speed). The
  harmonic LFOs now also **follow their controls on notes already
  sounding** - the one setting that does; everything else is still taken
  when a note starts, as in the original.
- **The filter envelope's knobs** are full size.
- The header shows when the copy you are running was built.
- Moving a control now tells the host where an edit starts and ends, so
  automation records cleanly.

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

Download `ForTran-1.3.0.0.pkg` and open it. The installer offers each part
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
SHA-256: 892cd4b56d15ae16ebeceddead87afdb5c38aa73dab683e91f7a90675c35bc92
```

---

Copyright 2026 A. E. Cobley. CC BY-SA 4.0.
VST is a trademark of Steinberg Media Technologies GmbH.
