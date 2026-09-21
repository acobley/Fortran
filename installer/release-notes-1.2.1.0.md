# ForTran 1.2.1.0

A polyphonic additive/formant synthesiser for macOS, as a **VST3** and an
**Audio Unit**. Originally a Cakewalk DXi written in 2003; this is the port.

## What changed since 1.2.0.0

- **The maker is now "AE Cobley"**, in both formats — it was "A. E. Cobley".
  REAPER files an Audio Unit's presets under its name cut off at the first
  full stop, so every A. E. Cobley Audio Unit shared one REAPER preset list
  and showed the others' presets, which did nothing when chosen. The
  plug-in's identity codes are unchanged, so **projects saved with earlier
  versions still open** with ForTran in place.
- **The factory presets now install to
  `/Library/Audio/Presets/AE Cobley/ForTran`**, the folder both formats now
  look in.
- **The version is reported correctly.** Earlier builds told hosts they were
  1.0.0.1 (VST3) and 1.0.0 (Audio Unit) whatever the release; this one says
  1.2.1.0.

Nothing in the sound changed.

**After upgrading**, some things stay where earlier versions left them:

- The old factory preset folder, `/Library/Audio/Presets/A. E. Cobley/ForTran`,
  is not removed by the installer. Nothing looks there now; see
  *Uninstalling* to remove it.
- Presets **you** saved in a host that files them by maker are in
  `~/Library/Audio/Presets/A. E. Cobley/ForTran`. Move them to
  `~/Library/Audio/Presets/AE Cobley/ForTran` to see them again.
- In REAPER, AU presets you saved earlier stay in REAPER's old shared list
  and will not appear under the new name. The VST3 is unaffected.
- Some hosts list the plug-in under its maker; look under **AE Cobley**.

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

Download `ForTran-1.2.1.0.pkg` and open it. The installer offers each part
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
sudo rm -rf "/Library/Audio/Presets/A. E. Cobley/ForTran"   # up to 1.2.0.0
```

The last two lines take the factory presets with them. Anything you saved yourself
lives in your own `~/Library/Audio/Presets` and is untouched.

## Checksum

```
SHA-256: <paste from: shasum -a 256 installer/ForTran-1.2.1.0.pkg>
```

---

Copyright 2026 A. E. Cobley. CC BY-SA 4.0.
VST is a trademark of Steinberg Media Technologies GmbH.
