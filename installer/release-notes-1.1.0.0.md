# ForTran 1.1.0.0

A polyphonic additive/formant synthesiser for macOS, as a **VST3** and an
**Audio Unit**. Originally a Cakewalk DXi written in 2003; this is the port.

## New in this release

**Portamento, and it is properly polyphonic.** The Glide knob on the right of
the panel sets the time; zero is off, which is the default, so nothing you
saved before this sounds different.

Each new note glides from the **nearest pitch of the chord that was sounding
when the current chord began**, and each of those pitches is used once - so
C-E-G to D-F-A is three glides of a tone, moving in parallel, rather than
three notes fanning out of whichever pitch happened to be last. Notes struck
within 30 ms of each other count as one chord; further apart than that and
each note glides from the one before it, which is what a played line wants.

The glide is **constant time** - every glide takes as long as the knob says,
whatever the interval, so a chord change arrives together - and **even in
semitones**, so an octave down sweeps at the same rate as an octave up. The
knob is squared like the envelope times: its centre is 500 ms of a two-second
maximum.

**Universal binary.** 1.0.0.1 was built for Apple Silicon only while its
installer claimed both architectures, so on an Intel Mac - or in any host
running under Rosetta - it installed cleanly and then loaded nothing. This
build is arm64 + x86_64.

## What it is

Ten sine partials, each with its own level, phase, invert and ADSR; ten
formant envelopes over them; a sweepable filter with four switchable shapes
(low pass, high pass, comb, grass); amplitude and filter envelopes; vibrato,
tremolo and a per-harmonic LFO; sixteen voices.

## Installing

Download `ForTran-1.1.0.0.pkg` and open it. The installer offers the two
formats separately:

    /Library/Audio/Plug-Ins/VST3/ForTran.vst3
    /Library/Audio/Plug-Ins/Components/ForTran.component

Quit your DAW first - a running host holds the old copy open. Signed with a
Developer ID and notarised by Apple, so it installs without any
Privacy & Security detour. Installing over 1.0.0.1 replaces it; your projects
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
```

## Checksum

```
<paste: shasum -a 256 installer/ForTran-1.1.0.0.pkg>
```

---

Copyright 2026 A. E. Cobley. CC BY-SA 4.0.
VST is a trademark of Steinberg Media Technologies GmbH.
