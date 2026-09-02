# Port checklist — ForTran

Working copy for the ForTran port. Phases 1 and 2 are done; the build is
Andy's step at each phase boundary.

`PORTING-GUIDE.md` is the reference behind every step; the section numbers
here point into it. Phases end where a **build** is needed — those are the
handoffs, because the assistant session reaches this Mac through a Linux VM
and cannot run cmake, Xcode or the validator.

**Legend** — 🧑 you · 🤖 assistant

---

## Decisions to record

Fill these in during Phase 1 and do not change them afterwards.

| | Value |
|---|---|
| Plug-in name | `ForTran` |
| Effect or instrument | **`aumu`** — a genuine polyphonic synth |
| VST3 category | `Instrument\|Synth` (`PlugType::kInstrumentSynth`) |
| AU subtype (4 chars, unique per plug-in) | `FTrn` |
| AU manufacturer (4 chars, same across your plug-ins) | `AECo` |
| Bundle id, VST3 | `audio.fortran.vst3` |
| Bundle id, AU | `audio.fortran.audiounit` |
| Processor class UID | `0x3B4723D5, 0xF76D4616, 0xAA9D9320, 0x43C2D59E` |
| Controller class UID | `0x29D6D36A, 0xB7EC46DB, 0xB03377F9, 0x8A8D647D` |
| Buses | no audio in; one stereo out |

> The four-character codes and both class UIDs are permanent once you ship a
> build. Change one and every existing session loses the plug-in.

---

## Phase 1 — Reconnaissance  (no code written)

- [x] 🤖 Read the whole DXi tree
- [x] 🤖 **Is it really an effect?** Does the loop read its input buffer? Does
      anything read the MIDI queue? Yes/no ⇒ audio effect, and the whole MFX
      layer is deleted rather than ported *(guide §1)*
- [x] 🤖 **Which processing loop is real?** Diff the near-identical copies;
      port the latest, identified by the features only it has *(§1)*
- [x] 🤖 Parameter inventory — every parameter, **both** ranges (external from
      `Parameters.h`, internal from `MapToInternal`), defaults, and any label
      that lies about what it does *(§4)*
- [x] 🤖 Dialog geometry — solve the dialog-unit scale from a control whose
      bitmap size is known, **verify against a second control**, extract every
      position as data *(§6)*
- [x] 🤖 Read `resource.h` — controls listed but absent from the shipped
      dialog are cut features, sometimes worth reinstating
- [x] 🧑 Confirm effect vs instrument, and choose the four-character codes
- [x] 🤖 Note anything the property page did *itself* — link switches, timers,
      readouts — these have no parameter behind them and are easy to miss

**Exit:** the table above is filled in and the parameter list is agreed.
This phase is cheap to redo and expensive to get wrong. Don't rush it.

---

## Phase 2 — Scaffold, and the first build

- [x] 🧑 `cp -R ~/DXi-DEv/vst3-port-template ~/DXi-DEv/<PlugIn>-VSTi`
- [x] 🤖 Edit the `PLUG-IN IDENTITY` block in `CMakeLists.txt` — and nothing
      else in that file
- [x] 🤖 Fill in `resource/au-info.plist` (every `CHANGE ME`), and make
      `AudioUnit SupportedNumChannels` match what `setBusArrangements` will
      accept *(§9)*
- [x] 🤖 Generate two fresh class UIDs
- [x] 🤖 Convert artwork `res/*.bmp` → `resource/*.png`, resolving the
      magic-colour transparency key to real alpha *(§3)*
- [x] 🤖 Write the seven stubs: `<PlugIn>IDs.h`, `<PlugIn>Params.*`,
      `<PlugIn>Processor.*`, `<PlugIn>Controller.*`, `<PlugIn>Entry.cpp`,
      `version.h` — **no DSP, no editor yet**
- [x] 🤖 `processContextRequirements.needTempo ()` in the processor's
      constructor, if it will use tempo *(§7 — silent failure otherwise)*
- [x] 🧑 `./setup-xcode.sh` — first configure fetches ~250 MB, takes minutes
- [x] 🧑 Build. **Milestone: a silent plug-in that passes the validator.**  ✅ 30 Aug

**Exit:** validator clean. Almost everything structural surfaces here, while
there are hundreds of lines to search rather than thousands.

---

## Phase 3 — Parameters  *(done during phase 2: the table is generated)*

- [x] 🤖Parameter table with all three ranges and a `toInternal()`
      reproducing `MapToInternal` exactly *(§4)*
- [x] 🤖`kIsBypass` added; any DXi "enabled" switch kept **separate**, with
      its original default
- [x] 🤖Enum step counts preserved even where values repeat
- [x] 🤖Never index the table with an unchecked `ParamID` — `kBypass` is 1000
- [x] 🤖`USTRING(...)` for every title and units — a null segfaults the
      validator, not your plug-in *(§7)*
- [x] 🧑 Build; the validator exercises parameter round-tripping hard  ✅ 30 Aug

---

## Phase 4 — DSP

- [x] 🤖Ported line for line, **free of SDK types** so it compiles standalone
- [x] 🤖Resist improving it — odd gain staging and apparent bugs are the
      sound *(§5)*
- [x] 🤖Fix only the genuinely broken: `sizeof(pointer)` in a `memset`,
      allocation on the audio thread, unbounded feedback/resonance, no
      denormal handling, 32-bit-float only, stereo only *(§5)*
- [x] 🤖Per-sample smoothing on the same parameters the DXi interpolated
- [x] 🤖`getTailSamples()` rather than porting the tail readers
- [x] 🤖Standalone tests that actually run — tap positions, quantisation
      grids, mapping functions at every realistic sample rate *(§8)*
- [x] 🤖Every deliberate behaviour change **proved a no-op** across normal
      settings before it is claimed
- [x] 🧑 Build and listen. First noise.  ✅ 30 Aug

---

## Phase 5 — Editor

- [x] 🤖Positions from the Phase 1 geometry — never placed by eye *(§6)*
- [x] 🤖Check `OnVScroll`: vertical sliders usually ran **top = maximum**
- [x] 🤖Port what the property page did itself — link switches issue a
      `performEdit` for the partner so the host records both sides
- [ ] 🤖 Value readouts always visible; **tooltips may never appear on
      macOS** *(§6)*
- [x] 🤖Anything the editor displays that the DSP computes comes from **one
      shared static function** both call *(§7)*
- [x] 🤖`editorDestroyed` compares upcast pointers, not `dynamic_cast` *(§7)*
- [x] 🤖Processor → controller values go via `outputParameterChanges`, never
      `sendMessage` from the audio thread *(§7)*
- [x] 🧑 Build and eyeball against a screenshot of the original  ✅ 1 Sep

---

## Parked — the MIDI controller paths

The arithmetic is asserted in `tests/SynthTests.cpp` against the DXi's own;
what these check is the VST3 **plumbing** — whether `IMidiMapping` actually
delivers the controller to the parameter. See PORTING-NOTES section 7.

- [x] 🧑 **Pitch wheel** — confirmed working in a host. The `IMidiMapping`
      route therefore works at all, which is most of what the other two were
      really testing.
- [ ] 🧑 **CC7 (channel volume)** — should ride notes that are already
      sounding, on a square-root law: CC7 at 32 is half the level of CC7 at
      127, not a quarter.
- [ ] 🧑 **CC10 (pan)** — and the new Pan knob, which **offsets** it rather
      than replacing it: with the knob centred, CC10 should behave exactly
      as it did before the knob existed. The law is now normalised the
      conventional way (unity at the extremes, centre 3 dB down), so a
      hard-panned note should no longer peak louder than a centred one.

## Phase 6 — Audio Unit and ship checks  ← CURRENT

- [ ] 🧑 Build the `<PlugIn>-au` scheme (Xcode generator only)
- [ ] 🧑 `auval -v aufx <subtype> <manufacturer>`
- [ ] 🧑 Validator against a **Release** build
- [ ] 🧑 Test in a real host
- [ ] 🧑 Before handing it to anyone: the installed `.component` holds a
      **symlink into your build tree** — replace it with the real `.vst3`
      bundle *(§9)*
- [ ] 🧑 Real signing identity, if it is going further than this machine

---

## Throughout

- [ ] 🤖 `PORTING-NOTES.md` written **as you go**, not at the end: the
      parameter mapping, every deliberate deviation and why, the UIDs and
      codes marked *never change*, and every trap hit with the symptom
      actually seen *(§10)*
- [ ] 🧑 Save build logs into the project folder — the assistant can read them
      there without you pasting
