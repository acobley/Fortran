# ForTran: DXi → VST3 port notes

Everything here refers to the original project in `../ForTran-DXI`
(Visual C++ 6, MFC, Cakewalk DirectX Plug-In Wizard, 2003).

The method is `../vst3-port-template/PORTING-GUIDE.md`; the worked
precedent is `../Spaceduo-VSTi`. This file records only what is true of
**ForTran**.

---

## 0. Identity — permanent once shipped

| | |
|---|---|
| Plug-in name | `ForTran` |
| Kind | **Instrument** — VST3 `Instrument\|Synth` |
| AU type / subtype / manufacturer | `aumu` / `FTrn` / `AECo` |
| Bundle id, VST3 | `audio.fortran.vst3` |
| Bundle id, AU | `audio.fortran.audiounit` |
| Buses | no audio input; **one stereo output** |

`AECo` is shared with SpaceDub — the manufacturer code identifies the
author, and only the subtype distinguishes the plug-ins. Class UIDs are in
`source/ForTranIDs.h`, freshly generated for this port.

---

## 1. What the plug-in is

Unlike SpaceDub — which registered as a synth but was really an effect —
**ForTran is a genuine polyphonic synthesiser**, and the port is an
instrument. The evidence is not ambiguous:

* `CForTran` derives from `CDXi`;
* `synthNote()`, `InitializeNoteEvent`, `ExpireNoteEvent`, `IsValidEvent`;
* a `NoteState` per sounding note holding its own ADSR envelopes, phase
  accumulators and LFO state;
* `m_adFreq[128]` — a note-number to frequency table;
* `MidiChan m_aMidiChan[16]`, each with 128 CCs, a patch number and a pitch
  wheel value.

Musically: ten harmonic oscillators, each with level, phase and invert; ten
formants, each with its own ADSR, level, enable and invert; separate
amplitude and filter envelopes; vibrato and tremolo LFOs; and a filter with
four independently switchable shapes (low pass, high pass, comb, "grass").

### Consequences

* The whole MFX/COM layer — `CSoftSynth`, `CInstrument`, `CStringMap`,
  `MediaParams`, `ParamEnvelope` — goes, as it did for SpaceDub. But unlike
  SpaceDub the **note machinery is real and must be rebuilt**: voice
  allocation, note-on/note-off, and pitch bend, against VST3's `IEventList`.
* **Polyphony was never bounded.** The DXi kept note state in the host's own
  event queue (`deque<DXiEvent>`, `de.pvData`), so there is no voice limit
  to copy — a VST3 processor has to own the voice pool and pick a number.
* **Program change selects the waveform.** In `Process`,
  `pns->eWaveShape = m_aMidiChan[chan].byPatch` — the MIDI patch number
  chooses sine / square / saw. That behaviour is worth keeping; it is the
  only way to reach two of the three oscillator shapes.
* **The DXi passed its audio input through to the output.** A DXi synth is
  handed an input buffer and ForTran `memcpy`s it to every non-mix output
  pin before synthesising. A VST3 instrument has no audio input, so this
  disappears. Nothing in the synthesis depends on it.
* **Multi-out is dropped.** The DXi published one output pin per waveshape
  plus a mix pin, selected by `_PARAM_MULTI_OUT`. This port is single stereo
  out, so all notes mix to one bus.
* **`Mode` ("Synth" / "Xform") is dead in the original.** Its only read is
  commented out (`ForTran.cpp:754`), so "Xform" was never implemented. The
  parameter is dropped rather than carried as a switch that does nothing.

---

## 2. Parameter map

**122 automated parameters** (`NUM_AUTOMATED_PARAMS`), plus three internal
ones that are not carried over (see below). Against SpaceDub's 29.

They are highly regular — 10 harmonics × 3, 10 formants × 7, three ADSR
banks × 5 — so `source/ForTranParams.cpp` is **generated** from the DXi's
own `Parameters.h`: both the enum order and the `m_aParamInfo` table are
parsed out of it, so the ranges and defaults are the original's by
construction rather than by transcription. Regenerate rather than hand-edit.

As in SpaceDub, `ParamDef` carries three ranges — VST3 normalised, the DXi
*external* range (so the displayed numbers match), and the *internal* range
the DSP is handed via `toInternal()`.

`ParamType` gains a fourth member, `Int`: the DXi used `MPT_INT` as well as
`MPT_BOOL`, `MPT_FLOAT` and `MPT_ENUM`.

### Deviations from the DXi's table

**1. Three of the four filter-shape parameters had wrong ranges. DEVIATION.**
`ForTranPropPage.cpp` builds the shape selectors as `NewSpin` cyclers. Low
pass is `SetRange(0,1)` with `Off / On` (line 352); the other three are
`SetRange(0,3)` with `Off / Add / Mult / Max` (lines 357–377). The DSP reads
all four as `(int)GetParamValue(...)`. `Parameters.h` declares:

| | internal range in `Parameters.h` | what the UI and DSP use | |
|---|---|---|---|
| `PARAM_LPSTATE` | 0..1 | 0..1 | **correct** |
| `PARAM_HPSTATE` | 0..**4** | 0..3 | wrong |
| `PARAM_COMBSTATE` | 0..**1** | 0..3 | wrong |
| `PARAM_GRASSSTATE` | 0..**4** | 0..3 | wrong |

So automating the comb shape could only ever reach `Off` and `Add` — `Mult`
and `Max` were unreachable except by clicking — while high pass and grass had
a fifth state with no name and no way to select it. Those three are 0..3 with
`stepCount 3` in this port. **Low pass is left at 0..1**; it is a two-state
switch and its declared range was right.

> An earlier draft of this port set all four to 0..3, having read the three
> `SetRange(0,3)` lines without noticing that the low-pass one above them
> says `SetRange(0,1)`. Worth recording as the kind of mistake the
> regularity of this plug-in invites.

**2. "Tremelo" is spelled "Tremolo".** A user-visible typo in the parameter
titles, not a name that means something else. Fixed.

### Not carried over

| DXi | Why |
|---|---|
| `_PARAM_MODE` | dead — its only read is commented out |
| `_PARAM_MULTI_OUT` | single stereo out (§1) |
| `_PARAM_FORMANT_` | **UI state, not a parameter** — see §3 |

`Bypass` (id 1000) is added, as VST3 hosts expect. **`Enabled` defaults to
ON** here — note the difference from SpaceDub, where it shipped off.

Checks on the generated table live in `tests/ParamTableTests.cpp`: dense
ids, no null title or units (a null segfaults `RangeParameter` — guide §7),
defaults inside their ranges, normalisation round-tripping, and all four
filter states reaching 0..3.

---

## 3. The interface

`IDD_PROPPAGE` is **556 × 232 dialog units**, MS Sans Serif 8 pt, so the
usual factors apply — the same as SpaceDub, being the same font:

```
x_px = x_dlu * 1.5        y_px = y_dlu * 1.625
```

The canvas is **840 × 388**, the true size of `res/background.bmp`. As with
SpaceDub the Windows dialog was smaller than its own backdrop (834 × 377)
and clipped it; the VST3 window shows all of it.

### The `.rc` geometry is fiction for 51 of the 71 controls

This is the single most important UI fact here, and it has no SpaceDub
equivalent. Most controls resize *themselves* at runtime, with `SWP_NOMOVE`,
so **only their x and y survive from the `.rc`; the declared width and
height are meaningless**:

| Controls | Declared | Actually |
|---|---|---|
| 29 "horizontal trackbars" | 36 × 29 DLU | **rotary knobs, 29 × 29 px** — `VMRotaryFaderCtrl::SetBitmap` → `SetWindowPos(..., SWP_NOMOVE)`, sized to `rotaryscale.bmp` |
| 22 `BS_PUSHLIKE` toggles | 18 × 12 DLU | **22 × 22 px** — `CButtonST::SizeToContent`, sized to `Green.bmp` |

The 29 rotaries are the envelope, LFO and harmonic-phase controls. This is
why the ten phase knobs, declared 36 wide on a 20 DLU pitch, do not in fact
overlap. A port that trusted the declared sizes would produce a panel that
looks nothing like the original.

Not resized, and so genuinely `.rc`-sized: the 13 vertical trackbars
(10 harmonic levels + cutoff/gain/poles), the four `NewSpin` filter-shape
cyclers (30 × 9 DLU), and the two `DrawArea` panels (138 × 63 DLU).

### The formant bank is edited through a selector

There are ten formants but only **one** set of five formant knobs on the
panel. The ten `IDC_FORMANT1..10` toggles choose which formant those knobs
edit — that is what `_PARAM_FORMANT_` (1..10) held. It is editor state, not
something to automate, so it does not appear in the parameter table; the
editor keeps it and re-points the five knobs plus the ENV and INV toggles.

### What the editor builds

`source/ForTranEditor.*` lays out **68 controls** — the 71 in the dialog less
the two `DrawArea` panels and the background static. Every position is
`x_dlu × 1.5, y_dlu × 1.625` computed in code from the `.rc` numbers; nothing
is placed by eye. A layout check confirms all 68 sit inside the 840 × 388
canvas with **zero overlapping pairs**, which is the useful test that the
conversion is self-consistent.

Sizes come from the runtime, not the `.rc`: knobs 29 × 29, toggles 22 × 22.

`source/ForTranControls.*` provides four control classes:

* **`FtKnob`** — the rotary. `CVMRotaryFaderCtrl` used `SetRange(30, 329)` in
  *degrees* with `degree = 360 - pos` and the dot at
  `centre + radius·(sin, cos)`, radius `knobW/2 - dotW/2 - 3` = 8 px. That
  works out as a conventional 299° clockwise sweep with the dead zone at the
  bottom. Drag vertically, shift for fine, wheel works.
* **`FtVSlider`** — drawn, not bitmapped: the DXi used a native Windows
  trackbar and shipped no slider artwork, so there is nothing to convert.
  Top is maximum, as the property page read it. **This is the one place the
  port has to invent an appearance rather than reproduce one.**
* **`FtToggle`** — `Green.bmp` off, `Red.bmp` on. Those are the DXi's
  colours, not a mistake.
* **`FtTextStepper`** — the four shape selectors, standing in for `NewSpin`.
  Drawn as **white text on a black plate with a hairline white border**:
  they sit on the pale left-hand panel and bare text on it could not be read.
  The plate fills the whole control rather than being sized to the string
  the way the value readouts are — the text cycles Off / Add / Mult / Max
  and a plate that changed width on every click would jump about.

  They are 18 px tall rather than the 9 dlu (14.6 px) the `.rc` gives them.
  Arial 11 plus the plate's border comes to almost exactly 14.6 px, i.e.
  flush to the edges with nothing to spare; there are 9 px of clear panel
  below them before the painted "LP HP Comb Grass" labels at y 207, so
  taking three of those is free.

### The vertical sliders sit a hair above their labels

Both groups of vertical sliders end within about two pixels of the label
painted underneath them: the harmonic columns finish 1.5 px above the numbers
1..10, and the filter sliders 1.6 px above "Cutoff Q Poles". Neither actually
overlaps — the `.rc` geometry is correct — but there is no daylight either.

The original survived that because a Windows trackbar draws a thin channel
and a small thumb, so almost nothing reached the bottom of the control's
rect. `FtVSlider` draws a full-width handle, which at minimum value sits
right on the lettering.

`kSliderBottomInset` (7 px) is the fix: the control keeps the rect the `.rc`
gives it, so the click target is unchanged, and draws into a slightly shorter
one. One constant covers both groups, rather than a different hand-picked
height for each — the squeeze is systematic, so the remedy should be too.

### The formant bank

Ten buttons, five knobs, two toggles — and only one set of knobs for ten
formants. The buttons are a radio group: clicking one re-points the five
knobs plus ENV and INV at that formant's parameters
(`setTag` + reload from the controller). That is exactly what
`_PARAM_FORMANT_` held in the DXi, and it stays editor state rather than
becoming a parameter.

The selector buttons carry tags 2000..2009 so they can never be mistaken for
parameter ids, and `valueChanged` intercepts them before anything reaches the
host.

### Value readouts

The DXi showed no numeric values anywhere. Unlike SpaceDub — whose property
page had a real tooltip handler formatting live values through `MyProps` —
`ForTranPropPage` contains no tooltip member, no handler and no message-map
entry. The tooltip sources are in the tree, and `MyToolTipCtrl.cpp` is even
in the `.dsp` so it compiles, but nothing outside its own files uses it: the
only live traces are a `EnableToolTips(TRUE)` inside the third-party
`NewSpin` control and a commented-out one in `VMRotaryFaderCtrl`, both
identical in the SpaceDub tree. Carried baggage from the shared codebase, not
a feature. **So nothing was lost here** — but neither was there any way to
read a value.

A readout was added, and it is **shared and transient** rather than one per
control. That is not a preference; the panel measurably has no room. Of the
42 knobs and sliders, **17 have nowhere to put a 40 × 12 plate** that does
not collide with a neighbour or cover the painted lettering, and shrinking
the plate only gets that down to 12. The only arrangement where all 42 fit
is one that paints over the panel's own labels — the harmonic numbers 1..10,
the ADSR names — which defeats the point of having them.

So one `FtValueDisplay` follows the pointer, appearing beside whatever is
hovered or dragged and clearing afterwards. Overlap stops mattering once the
reading is transient.

Two details:

* **The readout view spans the whole window width**, with the plate centred
  on the control inside it (`setAnchorX`). A narrow view clamped at the
  window edge drags the plate away from its control — the leftmost LFO knob
  ended up with its reading eighty pixels to the right. Anchoring inside a
  full-width view lets the plate slide along the edge instead, staying
  beside the control.
* **It stays up while a drag is in progress**, because the pointer routinely
  leaves the control it is dragging. `controlBeginEdit` / `controlEndEdit`
  track that; hover alone would make it flicker away mid-gesture.

Readings are per-parameter rather than uniform percentages where something
better exists: the cutoff control is an index into the filter's 552 bins, so
it reads in **hertz**; `Poles` reads as the integer count the DSP uses
(`1 + 10x`); the shape selectors read Off/Add/Mult/Max, and low pass Off/On.

**The 36 envelope time controls read as a time, not a percentage** — `200 ms`
below a second, `1.25 s` above it. The parameter table calls them
percentages because the DXi's did, but they set the length of a segment, and
since the range was widened the mapping is `rate² × maxSeconds` (Adsr.h) —
not proportional, so a percentage is close to meaningless on its own: 50 %
is a quarter of the range, not half of it. The classification lives in
`ForTranParams` as `envelopeTimeRole` / `envelopeTimeSeconds`, beside the
table rather than in the editor, and computes exactly what `Adsr` computes,
one-millisecond floor included, so the reading cannot drift away from the
envelope. It covers all twelve envelopes — amplitude, filter and the ten
formants — 12 attacks, 12 decays and 12 releases; sustain and level are
levels and still read as percentages. The formant block is ten repeats of
attack/decay/sustain/release/level, so the role is the offset modulo 5, with
`static_assert`s pinning that layout because the enum is generated.

At the factory 20 % this reads **200 ms** for attack and decay and **400 ms**
for release, which is what the DXi's linear mapping gave at the same
setting — the readout is a good check that the widening left the default
alone.

**The host agrees.** `ForTranParameter::toString` formats the same 36 ids as
times, so a DAW's generic editor and its automation lanes read `200 ms` too,
and `fromString` reads a time back, so a user can type one. Three details
that are easy to get wrong:

* **The units field must be emptied for these parameters.** `ParameterInfo`
  carries a units string the host puts *after* the value, and the table's is
  `"%"`. Leaving it would render `200 ms %`. Which unit applies depends on
  the value, so it belongs in the string, not in the field — `unitsFor()`
  in `ForTranController.cpp`.
* **`fromString` must be overridden as well as `toString`.** The base
  `RangeParameter` reads a leading number as a *plain* value, so typing
  `200 ms` back would be understood as 200 % and clamped to full travel.
  A bare number is taken as seconds; `ms`/`msec` and `s`/`sec` are honoured,
  case-insensitively.
* **The round trip has to be exact at the default.** The SDK validator
  round-trips every parameter through `getParamStringByValue` /
  `getParamValueByString` at its current value and warns above `1e-4`. It is
  exact at 20 % (`200 ms` → 0.2, `400 ms` → 0.2). Away from the default the
  millisecond rounding costs at most 0.06 % of the travel, which nothing
  checks but is worth knowing. `envelopeTimeToInternal` maps the whole band
  that `Adsr` floors to one millisecond back to 0 rather than part way up
  the travel, which is what keeps that figure small.

Formatting lives in one place — `formatEnvelopeTime` — used by both the
editor's readout and the parameter, so the panel and the host cannot show
the same control differently. `tests/ParamTableTests.cpp` checks the
classification (12/12/12, sustain and level excluded), the defaults, the
full round trip across every time parameter, the units a user might type,
clamping, and that 0.9999 s prints as `1.00 s` rather than `1000 ms`.

### The display panels

`IDC_DRAWAREA` (the filter response) and `IDC_DRAWWAVE` (the waveform) are
live-rendered displays rather than controls. Both are now built, as
`FtCurveView`, and **both are computed from the parameters alone**.

The response curve still is. The **waveform is now live**, which took a
little more work and is worth having: the DXi's was alive too.

`CForTranPropPage::OnInitDialog` sets a 50 ms timer, and `OnTimer` redraws
both panels — so the original ran at **20 fps**, and the waveform it drew was
the real synthesised output, captured in `synthNote` on a rising zero
crossing and fetched back through `MyProps`. While a note sounded you could
watch the formant envelopes open and the filter envelope sweep.

An earlier version of this port computed the waveform from the parameters
instead. That is not wrong, but it is a weaker thing: it shows one cycle as
it would be at the *start* of a note and never moves. The parameter-derived
version is still there and is what you see before anything has played, or
after touching a control — but a real frame overrides it whenever one
arrives.

**How the frame gets across.** `Synth` captures the mixed output into a
buffer on the audio thread, using the DXi's own trigger — a rising zero
crossing — and the DXi's own length, two periods of the most recent note.
Handover is a **seqlock**: the writer bumps a counter before and after the
copy, and a reader that sees an odd value, or a different one either side of
its own copy, discards the frame and tries again. No locks, no allocation on
the audio thread, and a dropped frame at 20 fps is invisible.

Getting it to the editor uses the only sanctioned route. A message from
`process()` is discarded by the host's connection proxy, so instead the
**editor asks**: a `CVSTGUITimer` at 50 ms has the controller send a request,
the processor answers it in `notify()` — which the host calls on the UI
thread, where replying is legal — and the reply carries the samples as a
binary attribute. Two messages per frame, 40 a second, only while the editor
is open.

One deliberate difference: this captures the **mixed** output, where the DXi
captured per note into a single shared buffer. With a chord each note
overwrote the last there, so the display showed whichever voice happened to
render last. The mix is simpler and closer to what you are actually hearing.

`tests/SynthTests.cpp` checks the capture: the frame is exactly two periods
at 440 Hz, it starts on a rising crossing, and two successive frames of a
steady note correlate at 0.99 — which is what confirms the trigger is holding
the picture still rather than letting it slide.

The response curve uses `FilterInfo::combinedCurve`, which folds the four
shapes across all 552 bins with **the same rules as the audio path**. The DXi
drew its curve from a second implementation living in the property page
(`ForTranPropPage.cpp:905-981`), which had a dangling-`else` of its own:

```c
case 3: if ((int)fLPState != 0)
            if (CombfData[i] > fData[i]) fData[i] = CombfData[i];
        else
            fData[i] = CombfData[i];
```

The `else` binds to the *inner* `if`, so it reads as "if LP on, assign
unconditionally; if LP off, do nothing" — the inverse of its indentation. The
graph the user saw therefore disagreed with the audio whenever Comb or Grass
was set to Max. One shared implementation removes the whole category.

The waveform samples the filter at bin 276 (440 Hz), where the filter
envelope starts before it sweeps.

Rendered offline and inspected: the response shows a resonant peak at the
cutoff — the point where the feedback term cancels the real part, clamped at
1.0 by the DXi's own limit — and the comb shows evenly spaced teeth, which is
what §6's bin-indexed comb should look like on a log-frequency axis.

There is **no control for `Enabled`** anywhere in the dialog, in the original
or here: the parameter exists and automates, but the DXi never put it on the
panel.

### The three envelope panels

**No DXi counterpart.** The original showed the filter response and the
waveform and nothing else; an envelope could only be read off four knob
positions. Three panels now sit down the right-hand side — amplitude,
filter, and whichever harmonic the formant selector is on.

**They draw the real envelope.** `traceEnvelope` (Adsr.h) drives the same
`Adsr` the voices use and plots what comes out, rather than drawing an
idealised trapezoid. So the picture carries what the sound carries:

* sustain sitting **just below** its control, never on it — the original's
  `case SUSTAIN: break;`;
* an inverted formant drawn upside down;
* a disabled formant as the flat **1.0** that `nextValue` really returns for
  one, level and invert ignored, annotated `off`;
* the millisecond floor at the bottom of the travel.

The one thing it does **not** draw is the one-sample overshoot past full
level. That is real, and `AdsrTests` still asserts it against the 2003
transcription — but its size is `1/attack` samples, so at 44.1 kHz it is
1.02 and lasts 23 µs, while at the plot's own 4 kHz trace rate it would draw
as **1.25**. Plotting it would show an artefact of the plot's resolution ten
times the size of the thing it claims to show, so the trace is clamped to
the envelope's legitimate range.

**Fixed scale, not normalised.** The other two panels rescale to fill
themselves; these must not, because an envelope's height *is* the Level
control. `FtCurveView::setFixedScale(1.f)` turns rescaling off. Level at
50 % draws at half height, which is the point.

**Each panel scales its own time axis** to attack + decay + a hold + release,
and prints the span top-right through `formatEnvelopeTime` — the same
function the readouts and the host use, so all three agree. The hold is a
quarter of the time attack and decay took, floored at 5 ms, so the sustain
level is always visible without a long envelope becoming mostly flat line.
A shared axis across the three panels was considered and rejected: with a
20 ms formant beside a 10 s amplitude envelope the short one becomes a
vertical line.

**Cost.** The trace runs at a fixed 4 kHz, not the host's rate — a 10 s
release at 96 kHz is a million steps to walk on every knob movement, twice,
three times over, on the UI thread. 4 kHz keeps the longest envelope under
150k steps while still leaving the millisecond floor four samples wide, so
the shortest settings still draw correctly. `traceEnvelope` also has a hard
40 s cap: a runaway loop here would freeze the host, not just the panel.
`AdsrTests` check 6f traces 288 specs — every combination of the degenerate
values — and asserts that all of them terminate, stay finite and stay in
range.

**Refresh is split.** `affectsEnvelopes` is separate from
`affectsDisplays` because the two sets of parameters are disjoint: tracing
three envelopes on every harmonic-slider movement would be wasted work, and
recomputing 552 filter bins on every envelope knob equally so. Selecting a
different formant redraws the third panel even though no parameter changed.

### The canvas grew to the right

840 → **1064**. `tools/make-background.py` appends a 224 px strip by
repeating the artwork's right-most column. That works because the gradient
has already flattened horizontally by the time it reaches the edge — the
last five columns of any row differ by at most a level or two — while it
still varies a lot vertically, so repeating the column continues the
vertical gradient exactly and leaves no seam. Extrapolating the horizontal
gradient over 224 px would have drifted. Nothing of the original artwork
moves; `kEditorWidth` and `WIDEN` have to stay in step.

### Controls with no SpaceDub equivalent

* **Rotary knobs.** `VMRotaryFaderCtrl` was dead code in SpaceDub and its
  artwork was carried across unused. Here it is the main control type, so a
  rotary control class has to be written.
* **Two `DrawArea` panels** — `IDC_DRAWAREA` (filter response) and
  `IDC_DRAWWAVE` (waveform). Live-rendered displays, not controls.
* **`NewSpin`** text cyclers for the four filter shapes — the same idea as
  SpaceDub's `SdTextStepper`.

### The one change to the artwork

`tools/make-background.py` builds `resource/background.png` from the DXi's
`res/background.bmp` with a single deliberate change: the painted word
**HARMONICS** sat at y 342..353, and the editor places the harmonic invert
buttons at y 333..355 — directly on top of it. In the Windows dialog the
buttons were the same size and the collision was the same; it is not
something this port introduced, but there is no reason to keep it.

The word moves down 20 px and the panel grows to suit, so the canvas is
**840 × 408** rather than the original 840 × 388. `kEditorHeight` has to
match.

It is a script rather than a hand-edited PNG so the change is legible, and
so it can be redone if the source artwork is ever revisited. Two things in
it are worth knowing, because both were got wrong first:

* **Do not insert a band of duplicated rows.** The obvious way to make room
  is to duplicate a row and push everything below it down. There is no row
  to duplicate: the labels ("Cutoff Q Poles", the formant digits, "Level")
  run right down to the panel's bottom border, so every candidate row has
  letter strokes crossing it, and duplicating one smears them into long
  vertical streaks. Instead the border band and the outer background below
  it are moved down, the gap is filled with an estimate of the panel
  interior, and only the HARMONICS label is relocated. Every other label
  keeps the position the original gave it.
* **Estimate that fill with a plain median, not a threshold.** The first
  attempt classified pixels as ink with an absolute cutoff. That cannot work
  across this artwork: the left-hand panel's background is darker than the
  centre one's, so a cutoff that excludes ink in the pale panel also
  excludes the *background* of the dark one, leaving a few anti-aliasing
  highlights to be averaged into a bright streak — which duly appeared as
  white bars under the word "Cutoff". A plain per-column median needs no
  threshold and cannot pick up ink, because ink is always the minority of a
  column.

The label's own crop is padded well past the ink for the same family of
reason: a box tight to the dark pixels clips the anti-aliasing, and erasing
from rows that still hold faint grey leaves ghost strokes behind.

### Artwork

Converted from `../ForTran-DXI/res/*.bmp`. `VMBitmap.cpp:295` takes the
transparency key from pixel (0,0), which is magenta — but the bitmaps have
been through a lossy round trip (`rotarydot.bmp` in particular is almost
entirely JPEG-smeared magenta), so an exact match is not enough. The
conversion keys on "magenta-ness" with soft alpha and un-premultiplies the
recovered colour against the key.

| Source | → | Size | Note |
|---|---|---|---|
| `background.bmp` | `background.png` | 840 × 388 | opaque |
| `rotaryknob.bmp` | `knob.png` | 28 × 28 | soft-keyed |
| `rotaryscale.bmp` | `scale.png` | 29 × 29 | soft-keyed |
| `rotarydot.bmp` | `dot.png` | 6 × 6 | soft-keyed; heavily damaged source |
| `Green.bmp` | `button_on.png` | 22 × 22 | **not** keyed |
| `Red.bmp` | `button_off.png` | 22 × 22 | **not** keyed |

`Green.bmp`/`Red.bmp` have pure black in the top and bottom rows only —
that is a border, not a transparency key, and keying it would have eaten the
button's edges. There are stale copies of five bitmaps in the DXi project
root; the `res/` copies are the ones that shipped, and `rotaryknob.bmp`
differs between the two (28 × 28 in `res/`, 29 × 29 in the root).

---

## 4. Bugs found in the original

Recorded as found; see the guide §5 for which of these to fix and which to
keep.

* **`Process` dereferences `pbufIn` after testing it for null.**
  `BOOL const bGenerateTail = (NULL == pbufIn);` is followed two lines later
  by `long cSampBuf = pbufIn->cSamp;`. It never crashed because the host
  always supplied a buffer, but the tail path as written cannot work.
* **The filter-state ranges** — §2, deviation 1.
* **`resource.h` has eleven stale control-ID aliases** whose numbers were
  later reused by live controls (`IDC_RADIO1` collides with
  `IDC_TREMELORATE`, and so on). `_APS_NEXT_CONTROL_VALUE` is 1053 while
  live IDs run to 1069, so the VC6 counter had been reset and adding a
  control would have silently duplicated an id. Nothing to port; it explains
  the mess.

### Green is on, red is off — the lamps were inverted

The twenty-two lamp buttons — the ten harmonic invert toggles, the formant
Enable and Inv toggles, and the ten formant selectors — shipped **the wrong
way round** until this was found by eye. Red meant on.

The DXi is unambiguous. Every one of them is set up identically in
`ForTranPropPage.cpp` (lines 613-699):

```cpp
m_HarmInv1.SetBitmaps(IDB_GREEN, NULL, IDB_RED, NULL);
```

`SetBitmaps` takes the "in" bitmap first and the "out" bitmap second, and
`CButtonST::DrawTheBitmap` picks between them (`BtnST.cpp:750`):

```cpp
if ((m_bIsCheckBox && bIsPressed) || (!m_bIsCheckBox && (bIsPressed || m_bMouseOnButton)))
    byIndex = 0;                             // the "in" bitmap - IDB_GREEN
else
    byIndex = (m_csBitmaps[1].hBitmap == NULL ? 0 : 1);   // IDB_RED
```

All twenty-two are `BS_AUTOCHECKBOX` in `ForTran.rc`, so `m_bIsCheckBox` is
true and the whole test reduces to "is it checked". **Checked draws green.**

Note what the parameter names in `SetBitmaps` mean, because it is the trap:
"in" and "out" are *mouse in* and *mouse out* — for an ordinary button they
are a hover effect. It is only the checkbox branch that reuses them as the
checked and unchecked states. Reading the call alone, `IDB_GREEN` first
looks like it could be either.

**This includes the ten formant selectors**, which were also inverted: the
selected formant is green in the DXi and the other nine are red. That was
not spotted at the same time as the enable and invert toggles, but it is the
same bug and the same line of `BtnST.cpp` settles it.

The fix is `tools/make-buttons.py`, which regenerates the two PNGs from the
DXi's `Green.bmp` and `Red.bmp` keyed by **what the state means**, not by
what colour it is. Nothing in the editor changed: the call sites already
asked for `button_off.png` and `button_on.png` correctly — the files behind
those names held the wrong lamps.

### The invert row is labelled, and the sliders take an alt-click mute

Two changes that came out of the lamp fix, because with the lamps finally
reading correctly the row still looked like a set of on/off switches.

**It is not one.** `IDC_HARMINV1..10` multiply the harmonic's level by −1
(`ForTran.cpp:789`) and never silence anything:

```cpp
fHarmSliders0 = GetParamValue(PARAM_HARM1);
if (GetParamValue(PARAM_HARMINV1) != 0.0)
    fHarmSliders0 = -1.0 * fHarmSliders0;
```

Measured through the real `Voice`, harmonic 3 against a fundamental: output
rms 0.08433 with invert off and 0.08426 with it on, while the *difference*
between the two signals is rms 0.10614. Nearly identical level, substantially
different waveform — which is what a phase flip is, and exactly why toggling
it reads as "nothing happened" if a mute is what you expect.

**1. The row now says "Inv".** The DXi left it unlabelled — the artwork
labels the *other* Inv toggle, in the Harminc ADSR panel, but not this row.
The word is not typeset: `tools/make-background.py` lifts the artwork's own
"Inv" and transplants it, so the font, weight and colour match exactly and
the script needs no font installed.

That transplant is not a paste. HARMONICS could be pasted as a rectangle
because it moves straight down within the same columns, where the gradient
is flat; this one moves 450 px left and 100 px up, from mid-cyan onto
near-white, so its rectangle would land as a conspicuous cyan patch. The ink
is keyed out instead: each pixel is modelled as navy laid over the local
background at some coverage, the coverage is recovered from how far the
pixel falls below a per-row background estimate taken from the clear columns
beside the word, and the ink is composited onto whatever is at the
destination. Anti-aliasing survives, because coverage is what anti-aliasing
is. The white drop shadow is deliberately dropped — it exists to lift the
word off mid-cyan, and the destination is almost white, which is how
HARMONICS already looks on that ground.

**2. Alt-click a harmonic slider to mute it**, alt-click again to bring the
level back. ForTran has no per-harmonic switch, so the only way to take a
partial out was to run the slider to the bottom and lose where it was.

It is a modifier rather than a new control on purpose. Ten more buttons have
nowhere to go — the panel is already full, which is why the value readout is
transient. And ten more parameters would change the table, which is the one
thing the port does not do lightly: a project saved by an earlier build
would not read back.

Shift was already taken, by the fine drag. Only the ten harmonic sliders get
it (`FtVSlider::setMuteable`, opted in from the editor) — "muted" means
nothing on cutoff, gain or poles. A muted harmonic's readout says **muted**
rather than "0 %", so the state alt-click puts it in has a name on screen;
that is the only discoverability the gesture has, short of a label the panel
has no room for.

Restoring goes to the remembered level, or to the parameter's own default if
the slider was already at the bottom when it was first alt-clicked.

### The phase knobs were already the feature nobody could see

The row of knobs between the harmonic sliders and the invert buttons is a
**real per-harmonic phase shift, and always was**. `ForTran.cpp:791`:

```cpp
fPhaseSliders0 = dPIm * (0.5 - GetParamValue(PARAM_HARMPHASE1));   // dPIm = -pi
```

| control | 0 % | 25 % | 50 % | 75 % | 100 % |
|---|---|---|---|---|---|
| phase | **−90°** | −45° | 0° | +45° | **+90°** |

Confirmed on a rendered voice, not just read off the source: measuring the
partial's actual phase across the knob's travel gives a total swing of 180°,
45° per quarter turn. The ten parameters have been exposed to the host as
"Phase Harmonic 1".."Phase Harmonic 10" since the parameter table was
generated.

Two things hid it, and both are now fixed:

* **The row had no label.** The artwork labels neither the sliders, the
  knobs nor the buttons. `tools/make-background.py` now sets the word
  "Phase" under the knob row. "Phase" appears nowhere in the artwork, so it
  is composed from the artwork's own letters — P from "Poles", a, s and e
  from "Release", and an **h built by laying the l of "Poles" over the n of
  "Inv"**, which is what an h is in this typeface. All the source letters
  come from nine-row label bands whose baseline is the last row, so glyphs
  cut from different parts of the panel share a baseline with no per-letter
  alignment.

  Setting it with a font was tried first and rejected on evidence: the best
  available faces (Carlito Bold, Liberation Sans Bold, Lato Bold, scored by
  overlap against the artwork's own "Inv") all render lighter and wider than
  the artwork's, and would have looked wrong 40 px from the transplanted
  "Inv".

* **The readout said "50 %".** A percentage says nothing about a phase, and
  reading 50 % at the centre is exactly what stopped the control being
  recognised. Both the panel and the host now read **degrees** — `+45°`, `0`,
  `-90°` — with `fromString` accepting a typed angle back. `ParamTableTests`
  checks the degrees against the DXi's own expression across the travel:
  worst error 1.1e-13°.

Two details that bite:

* **The degree sign is not ASCII.** `USTRING()` / `UString::assign` only
  carry ASCII, so U+00B0 is appended to the host's `String128` as a UTF-16
  code unit by hand. The editor's readout is UTF-8 and appends `\xc2\xb0`.
  `formatPhase` therefore returns the number alone and each caller adds its
  own sign.
* **The units field must be emptied** for these too, or a host renders
  `+45° %`.

Not changed: the range, the mapping, or the Invert buttons. The knob is the
DXi's ±90°, and Invert is a further 180° flip applied to the level rather
than the phase — between them the two controls reach the whole circle.
Widening the knob to ±180° and dropping Invert was offered and declined; it
would have changed the meaning of ten shipped parameters and removed ten
more, so no saved project would have read back.

### Cut features visible in `resource.h`

Three radio buttons (`IDC_RADIO1..3`), and formant IDs numbered up to 18
against the ten that shipped. Both were removed before release.

The ID numbers say a little more than that. VC6 leaves the old `#define`
behind when a control is deleted and a new one added, so a reused number is
a record of what replaced what:

```c
#define IDC_RADIO1        1016      #define IDC_FORMANT1001   1016
#define IDC_TREMELORATE   1016
#define IDC_RADIO2        1017      #define IDC_FORMANT12     1017
#define IDC_TREMELODEPTH  1017
#define IDC_RADIO3        1018      #define IDC_FORMANT13     1018
#define IDC_VIBRATORATE   1018
                                    #define IDC_FORMANT14     1019
#define IDC_VIBRATODEPTH  1019
```

Four extra formant controls occupied 1016..1019, and those four numbers were
reclaimed by exactly the four global LFO knobs — Tremolo Rate, Tremolo
Depth, Vibrato Rate, Vibrato Depth. Read as inference rather than as
something the code states, but it is the shape you would expect if
per-harmonic rate and depth controls were abandoned in favour of one global
tremolo and one global vibrato.

### The per-harmonic LFO that was started and never finished

Searched for deliberately, and the answer is that the **level** side was
half-built and the **phase** side never started.

**What exists.** `ForTran.h:87-89` declares three per-note arrays:

```cpp
double dFormantsLFOTheta[10];
double dFormantsLFOStep[10];
double dFormantLFODepth[10];
```

All three are written once per note (`ForTran.cpp:551-554`) and **never read
anywhere in either snapshot** — verified by grep across the whole tree. The
values are not placeholders; they are a considered design:

```cpp
pns->dFormantsLFOStep[i-1] = d2PI * 1 / (i * SamplesPerSec);   // period i seconds
pns->dFormantLFODepth[i-1] = 3.0 / i;                          // depth 3.0 down to 0.3
```

Harmonic *n* would wobble at 1/*n* Hz with depth 3/*n* — slower and gentler
the higher the partial. The line above them is the earlier attempt, still
commented out at `ForTran.cpp:552`:

```cpp
//pns->dFormantsStep[i]=d2PI*i/10*GetOutputFormat()->nSamplesPerSec;
```

**It is level, not phase.** The multiplier is already threaded through all
ten harmonic terms (`ForTran.cpp:1507..1543`):

```cpp
dTmp = dFormantLFO * pns->FormantsEnvelope[0].GetValue() * sin(dTheta) * fHarmSliders0 * FilterMagnitude[0];
```

`dFormantLFO` is a local, set to `1.0` at `ForTran.cpp:833` and again at
`865`, and never assigned anything else. It sits outside `sin()`, so it
scales **amplitude**. A depth of 3.0 only makes sense for an amplitude
multiplier too. The phase side has no hook at all: `dTheta` is built from
`n*dPtrTheta + FilterTheta[n] + fPhaseSliders[n]` with no modulation term
anywhere.

**Two more fingerprints.** `dFormantsLFOTheta[]` is zeroed at note-on and
never advanced — there is no counterpart to the global
`pns->dLFOThetaTremelo += pns->dLFOStepTremelo;` at `ForTran.cpp:1592`, so
the oscillators are frozen, not merely unread. And the two locals the
compiler complains about in `ForTran.plg` —

```
ForTran.cpp(866) : warning C4101: 'lT' : unreferenced local variable
ForTran.cpp(867) : warning C4101: 'Theta' : unreferenced local variable
```

— are declared two lines after `dFormantLFO=1.0;`, which is a counter and a
theta sitting exactly where a per-harmonic loop would have gone.

**And in the UI.** `ForTran.ncb`, VC6's browse database, retains three
property-page members that exist in no header, `.rc`, `.clw` or `.aps` in
either snapshot:

```
CForTranPropPage::m_Harm1TDepth
CForTranPropPage::m_Harm2TDepth
CForTranPropPage::m_Harm3TDepth
```

Three complete, consistently numbered names, stopping at 3. **T**remolo
**Depth** per harmonic is the obvious reading, and it matches the amplitude
conclusion above. There is no matching `TRate` ghost.

**Clean negatives**, all checked: no LFO, rate, depth or modulation
parameter is defined in `Parameters.h` — the enum and the `ParamInfo` table
are exhaustive and contain none, not even commented out. No dialog control
in `ForTran.rc` is left unwired. `Background.cpt`, the layered artwork, has
text layers only for `HARMONICS`, `Harminc ADSR`, `Envelope`, `Level`,
`Rate`, `DEpth`, `Depth`, `Tremelo` and `Vibrato` — no per-harmonic label
was ever drawn. `ForTran.clw` has nothing. So the feature never reached the
parameter layer, the dialog or the artwork.

One loose end worth recording: `ForTran.h:92` declares
`float bPhaseShift;` in `NoteState`, referenced nowhere in either snapshot.
The Hungarian `b` on a `float` says it began as a flag. It is the only trace
of anything phase-related that was started and dropped, and a single unused
member is not enough to say what it was for.

**Now finished.** See "The per-harmonic LFO, completed" in section 7.

---

## 5. DSP: the ADSR envelope

`source/Adsr.*` is a line-by-line port of `ADSR.cpp`. It has no SDK
dependency, so `tests/AdsrTests.cpp` builds and runs anywhere — and it
carries a transcription of the 2003 class, so the port is diffed against the
original sample for sample rather than merely looked at.

**Bit-identical across 432 settings × 8000 samples** (three sample rates ×
four attacks × three decays × four sustains × three releases, with a
note-off part way through). Max deviation 0. The comparison drives the
transcription from the port's own sample counts, so it pins the *shape* of
the envelope; the mapping from control to count is a deliberate deviation
and is pinned separately — see "The time range, widened" below.

### Quirks kept

* **Sustain is never assigned.** `case SUSTAIN: break;` leaves the value
  wherever decay last put it — at or just below the sustain control. Kept.
* **Release is twice attack** for the same control value. The original got
  that by scaling release by 2 and attack and decay by 1; the widened
  mapping keeps the ratio exactly (10 s against 5 s), and a test walks six
  control values checking release is still twice attack to within a sample.
* **The envelope overshoots 1.0 by one sample.** The phase test fires after
  the value is computed, so attack reaches 1.000104 at a normal setting
  before switching to decay. That is the original's, and it is not clamped
  away — the test asserts the original does it too, so the behaviour is
  pinned rather than assumed.
* **Times are fixed at note-on**, so they deliberately do not follow
  automation during a held note.

### The bug, and the fix

`ADSR::ADSR()` assigns float literals to `long` members:

```cpp
long Attack;  ...  Attack=0.0005;  Decay=0.0005;  Release=0.000005;
```

Every one truncates to **zero**, and `GetValue()` divides by them with no
guard. The controls make this reachable at runtime too — attack and decay
are percentages whose minimum is 0 — and the first sample of such a note is
`0.0f / 0.0f`, a NaN that then propagates through the whole voice. The test
confirms it: **64 of 64 samples non-finite.**

The counts are floored at **one millisecond**. One *sample* is not enough,
and the first attempt at this port used one sample and was caught by the
test: because the phase tests fire a sample late, a count of 1 overshoots to
2.0 in attack and then leaves decay at **-1.0**, which sustain holds for the
rest of the note — a full-level phase inversion, worse than the NaN it
replaced. A millisecond bounds the overshoot at 1.021 and lets decay reach
its sustain level normally.

A millisecond is below anything reachable in practice — the control is a
percentage and 1 % is already 10 ms, so the floor only engages in the bottom
0.1 % of its travel, which is exactly where the original produced NaN. Above
that the counts are untouched, which is what the 432-setting comparison
verifies.

Two members the DXi never initialised — `SamplesPerSec` and `EnvLevel` — are
initialised here. The original relied on every caller setting them, and every
caller did.

### The time range, widened

This is a **deliberate departure from the DXi**, made on request, and the
only one in the envelope beyond the millisecond floor above.

The original mapped the control linearly: `time = rate × 1 s` for attack and
decay, `rate × 2 s` for release. That gives a longest possible note of about
3 seconds and no way to build a slow pad. The mapping here is

```
time = rate² × maxSeconds        // 5 s attack and decay, 10 s release
```

`kMaxAttackDecaySeconds` and `kMaxReleaseSeconds` are public constants on
`Adsr`, so the ceiling is changed in one place.

**Why squared rather than a linear ×5.** Every time control in the parameter
table defaults to 20 %, and at 20 % the square law returns
`0.2² × 5 = 0.2 s` and `0.2² × 10 = 0.4 s` — which is exactly what the
DXi's linear mapping gave at the same setting. So the **factory patch is
unchanged**, to the sample, at every sample rate; a linear ×5 would have
made the default five times slower. Test 1b asserts this against the
transcription rather than trusting the arithmetic. The square law also puts
the fine resolution where it is wanted: below 20 % the control is finer than
the DXi's, above it, longer.

**What did change.** Any setting other than 20 % now means a different time
than it did on the DXi: 50 % was 0.5 s and is now 1.25 s, 100 % was 1 s and
is now 5 s, 5 % was 50 ms and is now 12.5 ms. Nothing was ported from a
saved DXi patch, so there is nothing to migrate, but a ForTran-VSTi session
saved *before* this change will read back slower.

The range applies to all twelve envelopes on a voice — amplitude, filter and
the ten formants — because they are all the same class. Longer releases mean
voices stay live longer, so the fixed 16-voice pool steals sooner at extreme
settings; that is the expected trade and the pool handles it (SynthTests 2).

The readouts still show the control as a **percentage**, as the DXi's panel
did, not as a time. Showing milliseconds would now be more informative,
since the mapping is no longer proportional — not done, and worth
considering.

---

## 6. DSP: the filter

`Filter.cpp`/`Filter.h` — 44 KB, the largest pair in the DXi — are **not
DSP**. They are the DirectShow COM plumbing (`CFilter`, `CFilterInputPin`,
`CFilterOutputPin`, over `CBaseInputPin`) that the plug-in wizard generated.
`ForTran.cpp` never mentions `CFilter`. The whole file goes, like SpaceDub's
MFX layer.

The real filter is `FilterInfo`, and it is not a time-domain filter at all.
ForTran is additive, so the "filter" is a magnitude and a phase looked up
**per partial**: 552 bins on a 1/48-octave grid — a quarter of a semitone —
with bin 276 pinned to 440 Hz, spanning 8.18 Hz to 23.3 kHz. Four shapes are
computed over that grid, and each of a note's ten harmonics reads the shape
at its own bin. The offsets are `12·log₂(n)` semitones rounded to the nearest
semitone and multiplied by four; the rounding is audible and is kept.

`source/FilterInfo.*` is a line-by-line port, checked bin for bin against a
transcription of the original in `tests/FilterInfoTests.cpp`: **identical
across 96 cutoff × gain × pole settings × 4 shapes × 552 bins**, max
deviation 0.

### Three controls that did nothing

`CalculateFilters()` is what turns the cutoff, gain and pole settings into
tables. Its call sites in the DXi are:

| Site | |
|---|---|
| `FilterInfo.cpp:39` | the constructor |
| `ForTran.cpp:448` | **commented out** |
| `ForTran.cpp:767` | **commented out** |
| `ForTran.cpp:1683` | live — inside `ChangeFilter()` |

and `ChangeFilter()` has exactly one caller: `ForTranPropPage.cpp:889`, the
**property page**, reaching into the DSP through the `MyProps` back-channel.

That call is the first line of `OnTimer`, and `OnInitDialog` does
`SetTimer(0, 50, NULL)` — so the tables were rebuilt **twenty times a
second**, unconditionally, for as long as the editor was open. All four
shapes, 552 bins, about 44,000 transcendentals a second on the UI thread.

> An earlier draft of these notes said the rebuild happened "when the user
> moved a control". It did not — it happened on a timer regardless. The
> user-visible consequence is the same, and so is the decision below, but the
> mechanism was described wrongly.

So in the shipped plug-in the tables were built once with `Cutoff = 440`,
`Feedback = 0.5`, `Poles = 4`, then rebuilt continuously while the editor was
open — and never again once it closed. With the editor closed — a rendered project, a freshly
loaded session — **Filter Cutoff, Filter Gain and Filter Poles did nothing**,
and automating them did nothing either.

This is the mirror image of SpaceDub, which used `MyProps` to read *from* the
DSP. VST3 has no such back-channel in either direction, so the behaviour
cannot be reproduced even deliberately.

**Decision: the three controls are live.** The processor recalculates when
any of them changes. This is a change of sound against a DXi session rendered
with the editor closed, but nobody could have depended on that: it turned on
whether a window happened to be open.

**Where the recalculation happens.** All four shapes are 552 bins of `pow`,
`atan` and `sin` — about 2,200 transcendentals. It runs at the top of
`process()` and only when a control has actually moved (`needsRecalculation`),
never per sample and never per note. That is a bounded spike on the audio
thread while a knob is moving, and nothing at all otherwise.

### Fixed

* **`SetCutoff` read one past the end.** `Frequencys[(int)(fCutoff*552.0)]`
  with `fCutoff` at the top of its travel is index 552 of a 552-element
  array. Clamped. Reachable, and now that the control is live, reachable
  often.
* **`atan(RI/RR)` could be 0/0.** NaN there propagates into every partial.
  Guarded; the magnitude clamps are written `if (!(mag <= 1.f))` so they trap
  NaN as well as overshoot.
* **The harmonic lookup was unguarded.** `GetLowPassValues` tested
  `Offset > 552` — off by one — and the high-pass, comb and grass lookups had
  no test at all. In the shipped plug-in this was harmless: the caller clamps
  its index to at most 276 and the largest harmonic offset is 160, so the
  worst reachable index was 436. The clamp now lives in the lookup, where a
  change to the caller cannot undo it.

### `atan` where `atan2` was meant — a 180° phase flip mid-sweep

**Fixed**, on request, as a deliberate deviation. The evidence is recorded
because it took a while to find and should not be re-derived.

The low-pass and high-pass phases come from the original's

```cpp
LowPassTheta[ix] = atan (dRILowpass / dRRLowpass);      // ForTran.cpp:87
```

`atan` of a ratio cannot know which quadrant the complex response is in: it
returns −π/2..+π/2, so wherever the real part changes sign the angle jumps by
π even though the true phase is perfectly continuous. And the real part does
change sign, repeatedly: it is `feedback + radius^poles · cos(poles·theta)`,
which oscillates as the pole count rises.

Sampled at the ten partials, low pass only, cutoff 50 %, gain 50 %, the
phase of partial 10 across four adjacent bins:

| bin | 69 | 70 | 71 | 72 |
|---|---|---|---|---|
| `atan` | −1.4666 | −1.5522 | **+1.4847** | +1.3544 |
| `atan2` | −1.4666 | −1.5522 | −1.6569 | −1.7872 |

Magnitude is **1.0** at all four, so that is a partial at full level
reversing sign in one sample. Measured as the change in a partial's complex
amplitude between adjacent bins - the honest measure, since a phase flip on
a silent partial is silent:

| poles control | worst jump, `atan` | worst jump, `atan2` |
|---|---|---|
| 0 % (1 pole) | 0.002 | 0.002 |
| 25 % (3 poles) | **1.000** | 0.024 |
| 50 % (6 poles) | **1.997** | 0.264 |
| 75 % (8 poles) | **1.997** | 1.074 |
| 100 % (11 poles) | 0.201 | 0.201 |

A jump of 2.0 is a unit-magnitude partial going from +1 to −1. Ten to
seventy of them lie across the 275 bins a full envelope sweep crosses.

**Why it is only being heard now.** Until the sweep was fixed (section 7)
the bin was pinned at the bottom of the table for most of every note, so the
crossings all happened inside the first half of the attack, where the
amplitude envelope is still small and the attack transient masks them. With
the sweep spread across the whole envelope, the same flips land in the
sustain and the release, in the open.

`atan2 (ri, rr)` is the fix and needs no other change: it returns the true
continuous angle, and its only discontinuity is the ±π wrap, which is a 2π
step and therefore inaudible in `sin()`. It is a **deviation from the DXi**,
and not a small one - it changes a rendered note by a peak of 2.3.

Two tests pin exactly how big the change is, because "we fixed the quadrant"
is a claim that has to be checkable:

* **It is a quadrant fix and nothing else.** 29,808 phases compared against
  the transcription of the original across cutoff × gain × poles: 9,017 of
  them differ by π, and every single one differs by **0 or π**, worst
  residual 1.5e-7 rad. Magnitudes stay bit-identical, as they were.
* **The flip is gone.** Worst change in a partial's complex amplitude
  between adjacent bins, over the range a sweep visits: **1.997 in the
  original, 1.074 in the port.**

The residual 1.074 is not a discontinuity. It is a genuinely steep filter -
eight poles with feedback - sampled on a quarter-semitone grid, where the
phase really does rotate that far between one bin and the next. That is what
the interpolation below deals with.

Note that `FilterInfoTests` had compared only **magnitudes** until now. The
transcription computed the phases and nothing ever looked at them, which is
why an inherited phase bug survived a test suite that reported "max deviation
0" on 96 settings × 4 shapes × 552 bins.

### The sweep is interpolated between bins

The `atan2` fix removed the sign flips but the clicks survived, quieter -
because the flips were only the loudest part of a more basic problem. The
DXi truncated the swept bin to an **integer**, so a sweep stepped through the
response a quarter-semitone at a time and every partial's magnitude and phase
changed abruptly, hundreds of times a note. At eight poles a legitimate,
bug-free response still rotates a partial's phase by 60 degrees between one
bin and the next.

The bin is fractional now, and the response is interpolated between the two
bins either side of it: magnitude linearly, phase **the short way round** -
lerping +3.1 to −3.1 the long way would sweep a partial straight through the
opposite phase, which is the very thing this is here to prevent.

**Measured spectrally**, which is what a click actually is. The synth is ten
partials of a 440 Hz note, so it has nothing of its own above 4.4 kHz: any
energy at 10 kHz is the sweep stepping. Worst 512-sample frame over a
three-second note with the filter sweeping:

| | 10 kHz energy |
|---|---|
| integer bin (the DXi) | 0.01362 |
| interpolated | **0.00014** |

Ninety-seven times less, about 40 dB. `VoiceTests` check 10 pins it with a
threshold between the two, an order of magnitude clear of each - and it
fails on the old code, which was checked rather than assumed.

**And it is cheaper than what it replaces.** `combine()` is four table
lookups and a fold per partial, and the DXi ran it *every sample* even
though the bin only changes every few hundred. It now runs only when the
integer bin moves, and what runs per sample is a multiply and an add per
partial with no transcendentals. Sixteen voices with low pass and high pass
engaged:

| | of one core |
|---|---|
| integer bin, `combine()` every sample | 7.62 % |
| **interpolated, cached** | **5.88 %** |
| interpolated, no cache | 9.96 % |

The cache is dropped at the top of every `render()` call. The filter tables
are rebuilt between blocks when a control has moved and a voice is never
told, so once a block is the cheapest way to be sure it is never drawing on
a table that has gone.

One more deviation goes with this. The DXi clamped a negative bin to **1**
(`if (Note<0) Note=1;`). Negative is now only reachable through the
envelope's one-sample overshoot past 1.0, and the clamp is to **0** - the
bottom of the sweep, where the rest of the sweep says the bottom is.

### A precision detail worth knowing

The original computes `Frequencys[ix]/Cutoff` as a **float** division and
only then widens to double. Doing that division in double instead — the
obvious way to write it — moves every low-pass and high-pass bin by about
6 × 10⁻⁸. Inaudible, but it is the difference between "identical" and
"nearly identical", and the bin-for-bin comparison caught it. The ratios are
computed in float deliberately.

### Folding the four shapes together

`synthNote` combines the four shapes into the ten harmonic magnitudes and
phases in a fixed order — low pass, high pass, comb, grass — across 600 lines
of hand-unrolled switch. `FilterInfo::combine` replaces it. An index audit of
all 21 unrolled blocks found **no** subscript errors; the damage is
structural, in three places.

Low pass always runs and is what initialises both arrays (magnitude 1, phase
0 when it is off). That must stay unconditional: it is the only write in the
region with no path around it, and turning it into a switch with a `default`
would leave the arrays uninitialised.

**1. `Max` did nothing. FIXED.** All three Max branches compute `max()` into
the shape's own scratch array and never copy it back — the line that would
have is commented out directly below each one:

```cpp
HPFilterMagnitude[i] = max (FilterMagnitude[i], HPFilterMagnitude[i]);
//  if (HPFilterMagnitude[i]>LPFilterMagnitude[i]) FilterMagnitude[i]=HPFilterMagnitude[i];
```

With low pass **on**, Max was a complete no-op. With it **off**, it assigned
the raw shape and discarded everything before it. The port does
`M = max(M, shape)` when something has contributed and `M = shape` otherwise
— what the property page's display code and those commented-out lines both
say was intended.

**2. The "has anything contributed yet" test named predecessors. FIXED.**
Each Add branch asks the question by testing particular earlier states, and
grass tests low pass and high pass but **never comb** — so with low pass and
high pass off, `Comb=Add, Grass=Add` discarded the comb entirely. Replaced by
a single `written` flag, which is what the ladder was reaching for.

**3. `Mult` and `Max` no longer touch the phase.** Mult multiplied radians by
radians and fed the result to `sin()`; Max never wrote phase at all. The port
leaves phase alone in both, so the rule is now statable: **Add accumulates
phase, Mult and Max do not.**

The difference this makes is narrower than it first appears. With low pass
off the phase accumulator starts at zero, so the original's multiply gives
zero — but so does leaving it alone, and a following Add revives it either
way. The two only part company when there is a phase to multiply, which means
when low pass is on.

`tests/FilterCombineTests.cpp` carries a transcription of the original,
bugs included, and asserts **both** halves: the port matches it across the 50
of 128 state combinations where the original was sound, and differs in
exactly the three ways above. An agreement test alone would not have caught a
fix that went too far.

### Kept

* **The comb is indexed by bin, not by frequency.** The frequency-based line
  is commented out directly above the one that replaced it, so the comb's
  teeth are evenly spaced in pitch rather than in Hz. That is what makes it
  sound like a comb across a harmonic series.
* `dDivPoles` is computed and never used, alongside the commented-out line it
  belonged to. Not carried over.

---

## 7. DSP: the voice

`source/Voice.*` is `CForTran::synthNote` plus the note-on half of
`InitializeNoteEvent`. Per sample: ten sine partials at `k·θ`, each with its
own ADSR, level, phase offset and filter magnitude; an amplitude envelope; a
tremolo LFO on level and a vibrato LFO on the phase step; then a fixed Moog
ladder low-pass and a ×5.

### Things that turned out to be nothing

* **The wave shape does not shape the wave.** `NoteState::eWaveShape` is set
  from the MIDI patch number and picks which *output pin* a note goes to. In
  `synthNote` the local `nShape` is assigned and never read — the only other
  mention is commented out. The oscillator is always a sum of sines. With one
  stereo output the whole mechanism disappears.
* **The per-formant LFO is dead.** `dFormantsLFOTheta`, `dFormantsLFOStep`
  and `dFormantLFODepth` are initialised for all ten partials at note-on and
  never read. `dFormantLFO`, which multiplies every partial, is set to 1.0
  and never changed. Not carried over.
* **`Filter.cpp` is not DSP** — see §6.

### The filter envelope's sweep — fixed

The filter envelope does not set a cutoff directly; it sweeps the **bin**
the ten partials are read at:

```cpp
FEnv = 551 * FilterCutoffEnvelope.GetValue();   // ForTran.cpp:884
Note = ANote - FEnv;                            // ANote = 69*4 = 276
if (Note < 0) Note = 1;
```

`551` is the size of the table; `276` is where A440 sits, which is where the
sweep starts. **They do not match.** The sweep runs *downwards* from 440 Hz,
so only the 276 bins below the starting point are reachable, but the
envelope is scaled to cover 551 of them. The bin therefore hits the bottom of
the table — bin 1, 8.29 Hz — as soon as the envelope passes

```
276 / 551 = 0.5009
```

and stays there for the whole of the rest of the envelope's travel. Measured
at the factory settings (A 20 %, D 20 %, S 75 %, R 20 %, level 100 %, 48 kHz):

| | |
|---|---|
| 0.000 – 0.100 s | bin sweeps 276 → 1 (440 Hz → 8.3 Hz), the first **half** of attack |
| 0.100 – 1.100 s | **pinned at the bottom** — the second half of attack, the whole decay, the whole sustain, and the first 0.1 s of release |

So at the default patch the filter envelope's **decay does nothing at all**,
and the attack appears to finish in half its stated time. How much of decay
is audible depends entirely on the sustain level, because the pin lifts when
the envelope falls back below 0.5009:

| filter sustain | of the attack, moving | of the decay, moving |
|---|---|---|
| 0 % | 50.1 % | 50.1 % |
| 25 % | 50.1 % | 33.5 % |
| 50 % | 50.1 % | 0.2 % |
| ≥ 50.1 % | 50.1 % | **0 %** |

This is the DXi's own behaviour — `551` and the `Note<0` clamp are both
verbatim — and it is almost certainly an oversight in the original rather
than a design decision. It was found by ear: the filter's decay sounded
delayed by roughly the length of the decay, which is exactly what a pin
lifting part way through a falling envelope sounds like.

### Fixed: the span is now the range that exists

**DELIBERATE DEVIATION**, made on request. `551` becomes `276`, so the
envelope's full travel maps onto the bins that actually exist:

```cpp
// Voice.h - both public, so the test can assert on them
static constexpr int   kFilterBinOrigin = 276;     // ANote = 69*4, A440
static constexpr float kFilterSweepBins = 276.f;   // the DXi's 551
```

The same measurement, at the factory settings and 48 kHz:

| | before (551) | after (276) |
|---|---|---|
| 0.050 s (in attack) | 60.9 Hz | 162.5 Hz |
| 0.100 s (in attack) | 8.3 Hz — **bottomed out** | 60.0 Hz |
| 0.200 s (attack ends) | 8.3 Hz | 8.2 Hz — bottom reached exactly at the peak |
| 0.225 s (in decay) | 8.3 Hz | 13.6 Hz — **decay is audible** |
| 0.400 s (sustain) | 8.3 Hz | 22.1 Hz |
| 0.700 s (in release) | 8.3 Hz | 60.0 Hz |

And **Filter Level means something again.** It scales the envelope before
the multiply, so with the span fixed it sets how deep the sweep goes.
Deepest bin reached at the factory ADSR:

| Filter Level | before (551) | after (276) |
|---|---|---|
| 25 % | 60.9 Hz | 162.5 Hz |
| 50 % | 8.3 Hz | 60.0 Hz |
| 75 % | 8.2 Hz — saturated | 22.1 Hz |
| 100 % | 8.2 Hz — saturated | 8.2 Hz |

Over its top half the control did nothing at all before; now every quarter
turn halves the floor by about an octave and a half.

`tests/VoiceTests.cpp` checks 8 and 8b pin this: the span must equal the
origin, a fully open envelope must land inside the table, and raising Filter
Level must keep deepening the sweep. Putting `551` back makes all three fail
— verified by doing exactly that, not assumed.

Not changed: the `100 %` default for Filter Level, and the `Note < 0 -> 1`
clamp, both as the DXi had them. The clamp is now only reachable by the
envelope's one-sample overshoot past 1.0.

### The per-harmonic LFO, completed

The one feature in this port that the DXi did not have working. What it did
have is set out in section 4: three per-note arrays, a `dFormantLFO`
multiplier threaded through all ten harmonic terms, and nothing ever written
to any of it. This finishes it.

**Twenty new parameters**, `kHarmLfoRate1..10` and `kHarmLfoDepth1..10`,
**appended after every DXi parameter** so no existing id moves. The table
goes from 122 to 142. `ParamTableTests` asserts the split — that `kEnable`
is still 0, `kHarmInv10` still 121, and the LFO block starts at 122 — because
inserting rather than appending would have shifted every id after the
insertion point and loaded every saved project's values into the wrong
controls.

**Depth defaults to zero, all ten.** That is what makes this additive rather
than a change: no existing patch sounds different, and `Voice` skips the
whole term when every depth is zero, so no existing patch costs a cycle
either. `VoiceTests` 9a proves the skip by changing the rate at depth zero
and requiring bit-identical output — if the term were computed and multiplied
by one, rate would still be read.

**The two code paths are proved equal.** `Voice::renderSample` has a plain
loop and an LFO loop. Test 9b sets one depth to `1e-30` — enough to switch
the voice onto the LFO path, far too small for `1 + 1e-30*x` to be anything
but exactly 1.0 — and requires the output to be bit-identical to the plain
path. That compares the branches against each other rather than trusting
that they were typed the same way.

**Rate is logarithmic**, 0.05 Hz to 20 Hz, defined once in `harmonicLfoHz`
in `Voice.h`. The DSP sets its oscillators from it and the panel's readout
reads its knobs through it — the split-component trap from the porting
guide, where the editor's private copy of a formula quietly diverges from
the processor's. A linear Hz knob spends nine tenths of its travel above
2 Hz and a linear *period* knob nine tenths below 1 Hz; neither is usable.

The **defaults are the DXi's own intended seeding**. Its abandoned
`dFormantsLFOStep[i-1] = 2*pi / (i * SamplesPerSec)` is a period of *i*
seconds, so harmonic *n* runs at 1/*n* Hz — and on a log knob that puts
harmonic 1 dead centre.

The abandoned `dFormantLFODepth[i-1] = 3.0/i` is **not** carried. It was
never read, so it is not "the sound"; and above 1 the multiplier goes
negative, inverting the partial twice a cycle. Depth is 0..1, applied as
`1 + depth * sin(theta)` — the same form as the global tremolo two lines
further down in the original.

**Cost**, measured with 16 voices sounding and all 160 oscillators running:
9.4 % of one core against 6.0 % with the LFOs off, on the Linux VM this
session reaches the Mac through. About half as much again for the extreme
case, nothing at all for the default patch.

Not modulated: **phase**. The DXi's hook is outside `sin()`, so it scales
amplitude; there is no phase hook anywhere, and the only trace of a phase
experiment is one unused `float bPhaseShift` in `NoteState`. Section 4 has
the evidence.

### The harmonic LFO panel

Twenty knobs, on a panel that already has no room for a permanent value
readout. They live on a panel that is summoned by a **"Harmonic LFO..."**
button in the strip below the third envelope display — the only place on the
window with space, since the DXi's artwork has none.

It is a `CViewContainer` child of the frame, **not a platform window**. A
real window would have to be parented, positioned, kept above the editor and
torn down by hand on three hosts' terms; a child view cannot be misplaced,
cannot outlive the editor, and draws in the same coordinates as everything
else.

**It floats and it drags.** A title bar, a plate, a border, a drop shadow so
it reads as being above the window rather than cut into it, and an `FtPanelGrip`
covering the title strip that moves the panel. `FtPanel::moveBy` clamps the
whole panel inside the frame - it is smaller than the window in both
directions, so there is no reason to let any of it hang off - and invalidates
the rectangle it is leaving as well as the one it is entering, or the panel
smears across the window as it is dragged. It keeps wherever it was put when
it is closed and reopened.

The grip is a **child view, not logic on the container**. VSTGUI routes a
drag's move and up events to whichever view consumed the mouse-down, and for
a container that bookkeeping is the container's own; a leaf child gets the
same straightforward treatment `FtVSlider` does. It is added FIRST so it is
the bottom-most child - hit testing runs topmost-first, so the Close button
that overlaps it is added after and wins where the two meet. The event
arrives in the PANEL's coordinates, so the grab point is measured against the
panel's current origin and the difference is the movement since the last
event however far the panel has already travelled.

### A container's `draw()` is never called

Worth its own heading, because it compiles, it looks right, and it produces
nothing at all.

The panel first shipped with no background and no border. The cause:

```cpp
class FtPanel : public CViewContainer { void draw (CDrawContext*) override; };
```

`CViewContainer::drawRect` paints its background and then walks its children
and **never calls `draw ()`**. The override was dead code. `CView::drawRect`
does call `draw ()`, which is why the habit is safe everywhere else in this
editor and wrong here.

The right hook is `drawBackgroundRect`, and it comes with a second
difference: `drawRect` has already translated the context by the container's
origin before calling it, so everything drawn there is in **local**
coordinates from `(0,0)` - not the parent-relative coordinates
`getViewSize ()` reports.

**Three ways out**, because a panel that traps you is worse than no panel:

* the **Close** button, in the title bar;
* a click **anywhere outside it** — an `FtScrim`, a transparent sheet the
  size of the window sitting under the panel and over everything else. It
  does two jobs: it catches the click, and while it is up nothing behind it
  can be dragged by accident;
* **Escape**, through `CFrame::registerKeyboardHook`. A hook rather than
  `onKeyboardEvent` on the panel, which would need the panel to hold
  keyboard focus — and hosts do not reliably give a plug-in editor's
  subviews focus.

Two details worth knowing:

* **A view's size is in its parent's coordinates.** The shared readout
  positions itself from `control->getViewSize()`, which is right for the
  controls sitting directly on the frame and two hundred pixels wrong for a
  knob inside a container. `showReadoutFor` now walks up the parent chain
  adding offsets until it reaches the frame.
* **The panel is added before the readout**, so the readout still draws on
  top of it. Z-order in VSTGUI is the order views were added.

The ten columns are on the harmonic sliders' own 30 px pitch, so column *n*
on the panel is under harmonic *n* on the main window. Rate reads in **Hz**
and depth reads **off** at zero rather than "0 %".

### The output ladder

A Moog ladder, "variation 2" from musicdsp.org — the same topology SpaceDub
used — sits on the way out, with its cutoff and resonance **hardcoded**:

```cpp
double f = 0.50 * 1.16;   fb = 0.5*(1.0-0.15*f*f);   ffff = 0.35013*(f*f)*(f*f);
```

No parameter reaches it. The Filter Cutoff control drives the spectral filter
of §6, not this.

**Its state lived on the plug-in, not on the note** — `m_in1..4` / `m_out1..4`
are `CForTran` members — so every voice was pushed through one filter in
sequence, each starting from the previous voice's state. With a single note
held that is invisible; only polyphony exposes it, and then the result
depends on the order the host happened to deliver the notes in.

**Decision: one ladder per voice.** For a linear filter that is identical to
filtering the mix, and it is better than the shared version in one respect —
each note starts from zero state instead of inheriting the previous note's
decay. `tests/VoiceTests.cpp` asserts two voices sum linearly, which is what
catches any accidental sharing.

### Fixed: three more divisions by zero

The LFO steps are built by dividing by the control:

```cpp
dLFOStepVibrato = d2PI * 1 / (GetParamValue(PARAM_VIBRATORATE) * SamplesPerSec);
dLFODepthVibrato = d2PI * 1 / ((1.0 - GetParamValue(PARAM_VIBRATODEPTH)) * SamplesPerSec);
dLFOStepTremelo  = d2PI * 1 / (GetParamValue(PARAM_TREMELORATE) * SamplesPerSec);
```

so a **rate of zero** — the bottom of a control whose minimum is zero —
divides by zero, and **vibrato depth at maximum** divides by `1 - 1`. All
three are reachable by dragging a control to its end stop. Floored at one
millisecond, matching `Adsr`.

Note what the rate control actually is: the LFO period is `rate × sampleRate`
samples, i.e. **`rate` seconds**. It is a period, not a frequency, despite
the name. Kept.

### Level — measured, and it is hot

The original's gain staging is `sqrt(cc7·vel/127²)·0.25` for "12 dB of
headroom", then the ladder's `×5`. Reproduced exactly, and **not adjusted**.
Measured on the factory patch, one note at velocity 100:

| | peak | above 0 dBFS |
|---|---|---|
| no filters | 2.95 | **+9.4 dB** |
| Low Pass on | 0.25 | −12.2 dB |
| High Pass Add | 1.91 | +5.6 dB |
| High Pass Mult | 1.68 | +4.5 dB |
| Comb Add | 2.17 | +6.7 dB |
| Grass Add | 1.01 | +0.1 dB |
| **all four Add** | **4.92** | **+13.8 dB** |

and with polyphony, no filters: 2 voices 4.4, 4 voices 7.2, 8 voices 12.9,
**16 voices 19.3 (+25.7 dB)**.

Nothing clips *inside* the plug-in — the buses are float and the output is
never clamped — so this is only audible where something downstream runs out
of headroom, typically the master or the interface. Pulling the track fader
down fixes it completely and losslessly. But it means the plug-in is
unusable at unity, and "Add" mode makes it worse because `combine()` **sums**
magnitudes with no clamp: four shapes in Add can reach a magnitude of 4
where one reaches 1. That is the original's rule, verified bin for bin.

**And it destroys velocity.** That is what made it worth fixing rather than
noting. `mVolume` is `sqrt (cc7·vel / 127²) · 0.25`, which is correct and is
the original's - the velocity reaches the voice and scales it. But the
factory patch on one note at C4:

| velocity | peak out | after a host clips at 1.0 |
|---|---|---|
| 1 | 0.29 | 0.29 |
| 8 | 0.83 | 0.83 |
| **16** | 1.17 | **1.000** |
| 64 | 2.35 | **1.000** |
| 127 | 3.30 | **1.000** |

Every velocity from 16 up lands above the ceiling and comes out at exactly
the same level. Only the bottom twelve per cent of the range does anything,
which no keyboard ever plays, so the instrument reads as having no velocity
response at all.

Underneath that, the square-root law halves the dB range on its own -
velocity 64 sits only **3.0 dB** below 127. That is the original's design and
is left alone; it makes the instrument gentler than a linear-velocity synth,
but it is playable and it is not what was breaking.

### The output trim

**Not from the DXi**, added because of everything above. `kOutputLevel`,
appended after the LFO block so no id moves. A plain gain on the mixed
output:

```
0 %      silence
25 %     -30 dB
50 %     -20 dB   <- the default, the knob's centre
75 %     -10 dB
100 %     0 dB    <- the DXi's own staging, unchanged
```

The **top of the knob is the original**, so nothing is lost and the DXi's
level is one turn away. The default is 20 dB below it. With that default,
the factory patch:

| voices | peak |
|---|---|
| 1 | 0.29 |
| 4 | 0.72 |
| 8 | 1.29 |
| 16 | 1.93 |

Single notes and four-note chords are clean; dense chords still want another
few dB, which is a notch of the knob. That is ordinary for a polyphonic
synth with no per-voice normalisation, and normalising by voice count would
be a compressor nobody asked for.

**Smoothed, not stepped.** A block-rate jump on a level control is a click,
and this one is on the mixed output where a click is at its most audible, so
the gain walks towards its target one sample at a time with a 10 ms time
constant - and keeps walking across the segment boundaries that sample-
accurate events split a block into. The first block after `setupProcessing`
snaps rather than ramping up from zero.

It reads in **decibels** on the panel and in the host, and `off` at the
bottom. The knob sits under the three envelope displays; making room meant
tightening the panel spacing from 24/24 to 6/14, so they now end at y 340
instead of 378.

### What the tests check

The voice is too large to diff against a transcription the way `Adsr` and
`FilterInfo` were, so `tests/VoiceTests.cpp` checks properties instead:
partials land on the note's own harmonic series and nowhere else (a
correlation at 440, 880 and a non-harmonic 660 Hz); silencing a harmonic
removes its frequency; 54 degenerate LFO settings and every envelope control
at zero stay finite; all 128 shape combinations stay finite and bounded; a
released note actually stops, within the time its release control asks for;
and two voices sum linearly.

### The voice pool, which has no original

A DXi synth never owned its voices. The host kept note state in its own
event queue (`deque<DXiEvent>`, `de.pvData`) and called `synthNote` once per
note per block, so there is **no voice limit to copy and no allocation
policy to port**. `source/Synth.*` is this port's own.

* **16 voices.** A choice, not a port. Each voice carries twelve ADSRs and
  its own ladder.
* **Stealing** prefers an already-released voice, oldest first, and falls
  back to the oldest voice of all.
* **Retriggering a held note reuses its voice** rather than stacking a
  second on the same pitch.
* **A note-on at velocity zero is treated as a note-off.** Files and hosts
  both send these, and a synth that ignores them hangs notes.
* **Events are handled even when the plug-in is disabled or bypassed**, for
  the same reason — the block is rendered silent, but a note-off is never
  dropped.
* **Note-off comes from the host.** The DXi released its envelopes when the
  sample counter passed the note's precomputed duration
  (`if (ix > lSampleNote)`), because a DXi was handed note *durations* up
  front. VST3 delivers a real note-off event, which is both simpler and
  better: a note held indefinitely no longer needs a `LONG_MAX` duration.

Events are applied **sample-accurately** — the block is rendered in segments
between them — rather than being quantised to the block boundary.

`tests/SynthTests.cpp` covers the properties that matter, the one worth
naming being a 4,000-event stream of deliberately unbalanced note-ons and
note-offs, after which `allNotesOff` must leave nothing sounding.

### MIDI controllers

Channel volume (CC7), pan (CC10) and the pitch wheel are carried over, routed
through `IMidiMapping`: the controller publishes `kMidiVolume`,
`kMidiPan` and `kMidiPitchBend` (ids 1001–1003) and
`getMidiControllerAssignment` points the host at them. They are deliberately
**absent from `getState`/`setState`** — live controller state, not settings.

All three follow the controller live rather than being latched at note-on.
That is the DXi's behaviour and it is easy to get wrong: `fVolume`,
`panToLRGain` and `fWheelPitch` are read in `synthNote`'s **prologue**, which
runs once per note *per block*, not in `InitializeNoteEvent`. Everything else
in this synth is latched at note-on; these three are not.

Two details are worth stating because the obvious implementation gets both
wrong:

* **The pan law is unity at centre, not 0.707.** `initTables` builds
  `L = sqrt(2(1-dx))`, `R = sqrt(2·dx)` with `dx = n/128`, so a centred note
  is at full level and a hard-panned one is **√2 louder**, not 3 dB quieter.
  A textbook cos/sin equal-power pan — which this port used at first — is
  3 dB down in the middle and wrong at both ends. The test measures
  1.230 centred against 1.740 hard left; 1.740 / 1.230 = 1.414.
* **The pitch wheel bends a whole octave each way.** `fWheel` is the wheel
  in −1..+1 and the multiplier is `2^fWheel`, so the end stops are ×0.5 and
  ×2 — six times the usual ±2 semitones. Measured: 220 Hz, 440 Hz, 880 Hz.

Channel volume follows `sqrt(cc7 · velocity / 127²)`, so it is a square-root
law, not linear: CC7 at 32 is half the level of CC7 at 127, measured at a
ratio of 1.992.

> **The pitch wheel is confirmed working in a host.** That settles the part
> that was actually in doubt: `IMidiMapping` does deliver a controller into
> a parameter, the processor does read it, and a sounding note does follow
> it. CC7 and CC10 travel the identical route and differ only in which
> parameter they land on, so what is left to check is their *laws* — the
> square root on CC7, and CC10's unity-at-centre pan — rather than whether
> anything arrives at all. See `PORT-CHECKLIST.md`.

### Not yet carried over

* **The waveform capture** (`pfWaveShape`, zero-crossing triggered) that fed
  the editor's `IDC_DRAWWAVE` panel. Editor work, phase 5.

---

## 8. Which copy is the original

`ForTran-DXI/` at the top level is the latest. `26-june-2003/ForTran/` and
`old/` are earlier snapshots; the June copy's dialog is byte-identical in
its header, so the layout had settled by then. There is only one copy of the
processing loop — unlike SpaceDub, which had two.

---

## 9. Traps hit in this port

**Logic reported the Audio Unit as unstable while the VST3 loaded fine.**
`process()` was writing silence with `std::memset` straight into
`channelBuffers32[ch]` with no null check. A host may call `process` with no
output buffers, with a bus whose channel array is null, or with individual
channel pointers null — a deactivated bus, a silent-output optimisation, or
simply a probe — and Logic probes considerably harder than Reaper, which is
why one host crashed and the other did not.

The SDK validator does **not** exercise this, so a plug-in can be
validator-clean and still take a host down. Every pointer in `process()` is
now checked. Worth remembering as a general rule: validator-clean is not the
same as host-safe.


**First build after the scaffold failed with "Bundle does not export the
required 'GetPluginFactory' function".** Not a factory problem: only
`ForTranParams.o` and `macmain.o` had been compiled. `CONFIGURE_DEPENDS`
re-globs through `ZERO_CHECK` at build time, so the build that first sees a
new source file regenerates `project.pbxproj` and then carries on with the
list Xcode loaded at build start. The link still succeeds — a bundle with no
factory is a valid dylib — and the SDK's post-build check is the first thing
to notice. Building a second time fixed it.

It then happened **again**, on the build that first saw `Voice.cpp`,
`Synth.cpp`, `VoiceParamsMap.cpp` and `FilterInfo.cpp` — this time as a page
of undefined symbols rather than a missing factory. Twice is a pattern, so
the template now carries a PRE_BUILD guard: the globbed file list is written
to `build/source-manifest.txt` at configure time and re-checked before every
build, so adding a file stops the next build with the names of what changed
and the command that fixes it.

**And the guard did not work.** It went past the editor files without a
murmur, because `CONFIGURE_DEPENDS` re-globs at build time through
`ZERO_CHECK` and rewrote the manifest and the project file together — so the
manifest always agreed with `source/` by the time the check ran. The globs
are now plain, which makes the manifest an independent witness; verified by
adding a file and watching the build stop. Written up in the guide, §7.
