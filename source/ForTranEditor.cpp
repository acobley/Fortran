//------------------------------------------------------------------------
// ForTran - editor implementation
//------------------------------------------------------------------------

#include "ForTranEditor.h"
#include "Voice.h"
#include "ForTranController.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace VSTGUI;
using namespace Steinberg;
using namespace Steinberg::Vst;

namespace ForTran {

namespace {
/** MS Sans Serif 8pt dialog base units. */
constexpr double kX = 1.5;
constexpr double kY = 1.625;

inline double px (double dlu) { return dlu * kX; }
inline double py (double dlu) { return dlu * kY; }

/** The runtime sizes the DXi's controls gave themselves, which are what you
    actually saw - not the sizes in the .rc. */
constexpr double kKnobSize   = 29.;   // rotaryscale.bmp
constexpr double kToggleSize = 22.;   // Green.bmp

/** Height of the four filter-shape selectors - see where they are built. */
constexpr double kSelectorHeight = 18.;

/** The ten harmonic columns share one x grid: 193 + 20n dialog units. */
inline double harmX (int n) { return px (193. + 20. * n); }
} // anonymous namespace

//------------------------------------------------------------------------
ForTranEditor::ForTranEditor (ForTranController* controller)
: VSTGUIEditor (controller)
, mController (controller)
{
	ViewRect rect (0, 0, kEditorWidth, kEditorHeight);
	setRect (rect);
}

//------------------------------------------------------------------------
CBitmap* ForTranEditor::bitmap (const char* name)
{
	auto it = mBitmaps.find (name);
	if (it != mBitmaps.end ())
		return it->second;
	auto bmp = makeOwned<CBitmap> (name);
	mBitmaps[name] = bmp;
	return bmp;
}

//------------------------------------------------------------------------
void ForTranEditor::registerControl (ParamID tag, CControl* control)
{
	mControls[tag] = control;
	if (mController)
		control->setValueNormalized (
		    static_cast<float> (mController->getParamNormalized (tag)));
	frame->addView (control);
}

//------------------------------------------------------------------------
void ForTranEditor::addKnob (ParamID tag, double xDlu, double yDlu)
{
	CRect r (px (xDlu), py (yDlu), px (xDlu) + kKnobSize, py (yDlu) + kKnobSize);
	auto* k = new FtKnob (r, this, static_cast<int32_t> (tag),
	                      bitmap ("scale.png"), bitmap ("knob.png"), bitmap ("dot.png"));
	k->setHoverListener (this);
	registerControl (tag, k);
}

//------------------------------------------------------------------------
void ForTranEditor::addKnobAt (ParamID tag, const CRect& r, CViewContainer* parent)
{
	auto* k = new FtKnob (r, this, static_cast<int32_t> (tag),
	                      bitmap ("scale.png"), bitmap ("knob.png"), bitmap ("dot.png"));
	k->setHoverListener (this);
	mControls[tag] = k;
	k->setValueNormalized (
	    static_cast<float> (mController ? mController->getParamNormalized (tag) : 0.0));
	if (parent)
		parent->addView (k);
}

//------------------------------------------------------------------------
FtVSlider* ForTranEditor::addVSlider (ParamID tag, double xDlu, double yDlu,
                                      double wDlu, double hDlu)
{
	CRect r (px (xDlu), py (yDlu), px (xDlu + wDlu), py (yDlu + hDlu));
	auto* s = new FtVSlider (r, this, static_cast<int32_t> (tag));
	s->setHoverListener (this);
	registerControl (tag, s);
	return s;
}

//------------------------------------------------------------------------
// GREEN IS ON. All twenty-two lamp buttons are BS_AUTOCHECKBOX driving a
// CButtonST set up as SetBitmaps(IDB_GREEN, NULL, IDB_RED, NULL), and
// DrawTheBitmap (BtnST.cpp:750) picks the FIRST of that pair when the box is
// checked - so checked is green and unchecked is red. The port shipped these
// the wrong way round until tools/make-buttons.py was written to generate
// them from the DXi's own Green.bmp and Red.bmp by meaning rather than by
// colour. See PORTING-NOTES section 3.
//------------------------------------------------------------------------
void ForTranEditor::addToggle (ParamID tag, double xDlu, double yDlu, bool radio)
{
	CRect r (px (xDlu), py (yDlu), px (xDlu) + kToggleSize, py (yDlu) + kToggleSize);
	auto* t = new FtToggle (r, this, static_cast<int32_t> (tag),
	                        bitmap ("button_off.png"), bitmap ("button_on.png"));
	t->setRadio (radio);
	registerControl (tag, t);
}

//------------------------------------------------------------------------
bool PLUGIN_API ForTranEditor::open (void* parent, const PlatformType& platformType)
{
	if (frame)
		return false;

	CRect frameSize (0, 0, kEditorWidth, kEditorHeight);
	frame = new CFrame (frameSize, this);
	frame->setBackgroundColor (CColor (40, 45, 50, 255));
	frame->open (parent, platformType);

	if (CBitmap* bg = bitmap ("background.png"))
		frame->setBackground (bg);

	// ---- harmonic columns: level slider, phase knob, invert toggle -----
	// All three share the x grid 193 + 20n; the sliders are 19 x 86 dlu at
	// y 86, the phase knobs sit at y 180 and the invert toggles at y 205.
	for (int n = 0; n < 10; ++n)
	{
		const ParamID level = static_cast<ParamID> (kHarm1 + n);
		// Alt-click a harmonic slider to mute it, alt-click again to bring
		// it back. Only these ten: "muted" means nothing on cutoff or poles.
		addVSlider (level, 193. + 20. * n, 86., 19., 86.)
		    ->setMuteable (static_cast<float> (paramDef (level).defaultNormalized ()));
		addKnob    (static_cast<ParamID> (kHarmPhase1 + n), 193. + 20. * n, 180.);
		addToggle  (static_cast<ParamID> (kHarmInv1 + n),   193. + 20. * n, 205.);
	}

	// ---- amplitude envelope (2x2 plus level to the right) --------------
	addKnob (kEnvAttack,  409., 17.);
	addKnob (kEnvDecay,   451., 17.);
	addKnob (kEnvSustain, 409., 52.);
	addKnob (kEnvRelease, 451., 52.);
	addKnob (kEnvLevel,   492., 25.);

	// ---- filter envelope (level BELOW and centred, unlike the others) ---
	addKnob (kFilterAttack,  89., 135.);
	addKnob (kFilterDecay,  131., 135.);
	addKnob (kFilterSustain, 89., 170.);
	addKnob (kFilterRelease,131., 170.);
	addKnob (kFilterLevel,  107., 201.);

	// ---- LFOs: vibrato pair then tremolo pair, pitch 39 not 42 ---------
	addKnob (kVibratoRate,  10., 17.);
	addKnob (kVibratoDepth, 49., 17.);
	addKnob (kTremoloRate,  88., 17.);
	addKnob (kTremoloDepth,127., 17.);

	// ---- filter sliders: cutoff, gain, poles (19 x 80 dlu) -------------
	addVSlider (kCutoff,    21., 135., 19., 80.);
	addVSlider (kFilterGain, 42., 135., 19., 80.);
	addVSlider (kPoles,      63., 135., 19., 80.);

	// ---- the four shape selectors --------------------------------------
	{
		struct Sel { ParamID tag; double x; int steps; bool twoState; };
		const Sel sels[4] = {
			{ kLowPassState,   21., 1, true  },
			{ kHighPassState,  54., 3, false },
			{ kCombState,      87., 3, false },
			{ kGrassState,    120., 3, false },
		};
		for (const Sel& s : sels)
		{
			// The .rc gives these 9 dlu - 14.6 px - which is exactly the
			// height of Arial 11 plus the plate's border, i.e. flush to the
			// edges. There are 9 px of clear panel below them before the
			// painted "LP HP Comb Grass" labels at y 207, so they get 18.
			CRect r (px (s.x), py (113.), px (s.x + 30.), py (113.) + kSelectorHeight);
			const bool two = s.twoState;
			auto* c = new FtTextStepper (r, this, static_cast<int32_t> (s.tag), s.steps,
			                             [two] (int step) -> std::string {
				                             if (two)
					                             return kLowPassStateNames[std::clamp (step, 0, 1)];
				                             return kFilterStateNames[std::clamp (step, 0, 3)];
			                             });
			registerControl (s.tag, c);
		}
	}

	// ---- the formant bank ----------------------------------------------
	//
	// Ten buttons, five knobs and two toggles. The buttons choose which of
	// the ten formants the knobs edit - there is only one set of knobs.
	{
		const double row1x[5] = { 414., 431., 448., 466., 483. };
		const double row2x[5] = { 414., 432., 448., 466., 484. };  // hand-nudged, not a typo
		for (int i = 0; i < 5; ++i)
		{
			CRect a (px (row1x[i]), py (182.), px (row1x[i]) + kToggleSize, py (182.) + kToggleSize);
			auto* ba = new FtToggle (a, this, 2000 + i, bitmap ("button_off.png"), bitmap ("button_on.png"));
			ba->setRadio (true);
			mFormantButtons[i] = ba;
			frame->addView (ba);

			CRect b (px (row2x[i]), py (196.), px (row2x[i]) + kToggleSize, py (196.) + kToggleSize);
			auto* bb = new FtToggle (b, this, 2005 + i, bitmap ("button_off.png"), bitmap ("button_on.png"));
			bb->setRadio (true);
			mFormantButtons[5 + i] = bb;
			frame->addView (bb);
		}

		// The five knobs. Their tags are set by retargetFormantControls().
		const double kx[5] = { 409., 451., 409., 451., 492. };
		const double ky[5] = { 105., 105., 140., 140., 105. };   // level is flush with the TOP row
		for (int i = 0; i < 5; ++i)
		{
			CRect r (px (kx[i]), py (ky[i]), px (kx[i]) + kKnobSize, py (ky[i]) + kKnobSize);
			auto* k = new FtKnob (r, this, 0, bitmap ("scale.png"), bitmap ("knob.png"), bitmap ("dot.png"));
			k->setHoverListener (this);
			mFormantControls[i] = k;
			frame->addView (k);
		}
		// INV and ENV, on their own row to the right of the knobs.
		CRect ri (px (491.), py (140.), px (491.) + kToggleSize, py (140.) + kToggleSize);
		auto* inv = new FtToggle (ri, this, 0, bitmap ("button_off.png"), bitmap ("button_on.png"));
		mFormantControls[6] = inv;
		frame->addView (inv);

		CRect re (px (515.), py (140.), px (515.) + kToggleSize, py (140.) + kToggleSize);
		auto* env = new FtToggle (re, this, 0, bitmap ("button_off.png"), bitmap ("button_on.png"));
		mFormantControls[5] = env;
		frame->addView (env);

		selectFormant (0);
	}

	// ---- the two display panels (IDC_DRAWAREA, IDC_DRAWWAVE) -----------
	{
		CRect resp (px (13.), py (49.), px (13. + 138.), py (49. + 63.));
		mResponseView = new FtCurveView (resp, CColor (120, 230, 160, 255));
		frame->addView (mResponseView);

		CRect wave (px (219.), py (20.), px (219. + 138.), py (20. + 63.));
		mWaveView = new FtCurveView (wave, CColor (120, 200, 255, 255));
		frame->addView (mWaveView);
	}

	// ---- the three envelope panels (no DXi counterpart) ----------------
	//
	// Stacked on the strip make-background.py adds to the right. Each draws
	// the shape its envelope really produces - traceEnvelope drives the same
	// Adsr the voices do - on a FIXED 0..1 scale, so the Level control shows
	// as height rather than being normalised away.
	{
		const CColor colours[3] = {
			CColor (255, 205, 120, 255),   // amplitude: amber
			CColor (140, 235, 170, 255),   // filter: the response panel's green
			CColor (185, 175, 255, 255),   // harmonic: violet
		};
		for (int i = 0; i < 3; ++i)
		{
			const CCoord top = kEnvPanelTop + i * (kEnvPanelHeight + kEnvPanelGap);
			CRect r (kEnvPanelX, top, kEnvPanelX + kEnvPanelWidth, top + kEnvPanelHeight);
			mEnvViews[i] = new FtCurveView (r, colours[i]);
			mEnvViews[i]->setFixedScale (1.f);
			frame->addView (mEnvViews[i]);
		}
	}

	// The button that summons the harmonic LFO panel, in the space below the
	// third envelope display. Nowhere on the DXi's own artwork has room.
	{
		// Four controls in a row under the envelope panels, each with its
		// name beneath it: the output trim, pan, portamento, and the ladder
		// switch. None of them has a place on the DXi's own artwork - the
		// original had no output control, no pan control and no glide.
		auto label = [this] (int centre, const char* text) {
			frame->addView (new FtLabel (
			    CRect (centre - kStripLabelHalf, kStripLabelTop,
			           centre + kStripLabelHalf, kStripLabelTop + 12),
			    text, kCenterText));
		};

		addKnobAt (kOutputLevel,
		           CRect (kStripCol1 - kKnobSize / 2, kStripRowTop,
		                  kStripCol1 - kKnobSize / 2 + kKnobSize, kStripRowTop + kKnobSize),
		           frame);          // a CFrame is a container like any other
		label (kStripCol1, "Output");

		addKnobAt (kPan,
		           CRect (kStripCol2 - kKnobSize / 2, kStripRowTop,
		                  kStripCol2 - kKnobSize / 2 + kKnobSize, kStripRowTop + kKnobSize),
		           frame);
		label (kStripCol2, "Pan");

		// Labelled "Glide" because "Portamento" does not fit in 48 px; the
		// parameter, the host's automation lane and the hover readout all
		// call it Portamento.
		addKnobAt (kPortamento,
		           CRect (kStripCol3 - kKnobSize / 2, kStripRowTop,
		                  kStripCol3 - kKnobSize / 2 + kKnobSize, kStripRowTop + kKnobSize),
		           frame);
		label (kStripCol3, "Glide");

		// Green is on, as everywhere. Centred on the knobs' own centre line.
		{
			const CCoord top = kStripRowTop + (kKnobSize - kToggleSize) / 2;
			auto* t = new FtToggle (
			    CRect (kStripCol4 - kToggleSize / 2, top,
			           kStripCol4 - kToggleSize / 2 + kToggleSize, top + kToggleSize),
			    this, static_cast<int32_t> (kLadderEnable),
			    bitmap ("button_off.png"), bitmap ("button_on.png"));
			registerControl (kLadderEnable, t);
		}
		label (kStripCol4, "Ladder");

		frame->addView (new FtTextButton (
		    CRect (kEnvPanelX, kLfoButtonTop,
		           kEnvPanelX + kEnvPanelWidth, kLfoButtonTop + 17),
		    "Harmonic LFO...", [this] () { showLfoPanel (true); }));
	}

	// The panel and its sheet, hidden, added before the readout so the
	// readout still draws on top of them.
	buildLfoPanel ();

	refreshDisplays ();
	refreshEnvelopes ();

	// Added last so it draws over every control. Full width, with the plate
	// inside sized to the string and centred on whatever is being touched.
	mReadout = new FtValueDisplay (CRect (0, 0, kEditorWidth, 15));
	mReadout->setVisible (false);
	frame->addView (mReadout);

	// The scope. 50 ms is what CForTranPropPage::OnInitDialog used -
	// SetTimer(0, 50, NULL) - so the display runs at the DXi's own 20 fps.
	mScopeTimer = makeOwned<CVSTGUITimer> (
	    [this] (CVSTGUITimer*) {
		    if (mController)
			    mController->requestWaveFrame ();
	    }, 50, true);

	frame->registerKeyboardHook (this);

	return true;
}

//------------------------------------------------------------------------
void ForTranEditor::selectFormant (int index)
{
	mSelectedFormant = std::clamp (index, 0, 9);
	for (int i = 0; i < 10; ++i)
		if (mFormantButtons[i])
		{
			mFormantButtons[i]->setValueNormalized (i == mSelectedFormant ? 1.f : 0.f);
			mFormantButtons[i]->invalid ();
		}
	retargetFormantControls ();
	// The third panel follows the selector, so switching formant redraws it
	// even though no parameter changed.
	refreshEnvelopes ();
}

//------------------------------------------------------------------------
void ForTranEditor::retargetFormantControls ()
{
	// Five consecutive parameters per formant - attack, decay, sustain,
	// release, level - then the ENV and INV flags in their own blocks.
	const int base = kFormantAttack1 + mSelectedFormant * 5;
	const ParamID tags[7] = {
		static_cast<ParamID> (base + 0), static_cast<ParamID> (base + 1),
		static_cast<ParamID> (base + 2), static_cast<ParamID> (base + 3),
		static_cast<ParamID> (base + 4),
		static_cast<ParamID> (kFormantEnv1 + mSelectedFormant),
		static_cast<ParamID> (kFormantInv1 + mSelectedFormant),
	};

	mUpdating = true;
	for (int i = 0; i < 7; ++i)
	{
		if (!mFormantControls[i])
			continue;
		mFormantControls[i]->setTag (static_cast<int32_t> (tags[i]));
		if (mController)
			mFormantControls[i]->setValueNormalized (
			    static_cast<float> (mController->getParamNormalized (tags[i])));
		mFormantControls[i]->invalid ();
	}
	mUpdating = false;
}

//------------------------------------------------------------------------
// The shared value readout.
//
// There is no room on this panel for a permanent readout per control: of the
// 42 knobs and sliders, 17 have nowhere to put one that does not cover the
// painted lettering - the harmonic numbers 1..10, the ADSR names. So one
// readout follows the pointer instead, appearing beside whatever is being
// hovered or dragged. Overlap stops mattering once it is transient.
//------------------------------------------------------------------------
std::string ForTranEditor::readoutTextFor (ParamID tag) const
{
	if (!mController || tag >= kNumParams)
		return {};

	const ParamDef& def = paramDef (tag);
	const double norm  = mController->getParamNormalized (tag);
	const double plain = def.toPlain (norm);
	const double inner = def.toInternal (norm);

	char buf[96] = {};

	switch (tag)
	{
		case kLowPassState:
			std::snprintf (buf, sizeof (buf), "%s: %s", def.title,
			               kLowPassStateNames[std::clamp (static_cast<int> (inner), 0, 1)]);
			break;

		case kHighPassState:
		case kCombState:
		case kGrassState:
			std::snprintf (buf, sizeof (buf), "%s: %s", def.title,
			               kFilterStateNames[std::clamp (static_cast<int> (inner), 0, 3)]);
			break;

		// The cutoff control is an index into the filter's 552 bins, so the
		// useful reading is the frequency it lands on, not a percentage.
		case kCutoff:
		{
			const int bin = std::clamp (static_cast<int> (inner * FilterInfo::kNumBins),
			                            0, FilterInfo::kNumBins - 1);
			std::snprintf (buf, sizeof (buf), "%s: %.0f Hz", def.title, mFilter.binFrequency (bin));
			break;
		}

		// DXi: SetPoles (1 + PARAM_POLES * 10), truncated.
		case kPoles:
			std::snprintf (buf, sizeof (buf), "%s: %d", def.title,
			               static_cast<int> (1.0 + inner * 10.0));
			break;

		default:
			if (def.type == ParamType::Bool)
			{
				std::snprintf (buf, sizeof (buf), "%s: %s", def.title, (inner >= 0.5) ? "On" : "Off");
				break;
			}

			// The thirty-six envelope time controls read as a time, not as
			// the percentage the DXi's table calls them. The mapping is not
			// proportional (Adsr squares the control), so a percentage here
			// tells you almost nothing: 50 % is a quarter of the range, not
			// half of it. Sustain and level are levels and fall through to
			// the percentage below.
			if (const double seconds = envelopeTimeSeconds (tag, inner); seconds >= 0.0)
			{
				char time[16] = {};
				formatEnvelopeTime (seconds, time, sizeof (time));
				std::snprintf (buf, sizeof (buf), "%s: %s", def.title, time);
				break;
			}

			// Glide reads as a time, and "off" at zero. The label under
			// the knob says Glide; the readout says Portamento, which is
			// the parameter's name in the host.
			if (tag == kPortamento)
			{
				const double seconds = portamentoSeconds (inner);
				if (seconds <= 0.0)
					std::snprintf (buf, sizeof (buf), "%s: off", def.title);
				else
				{
					char time[16] = {};
					formatEnvelopeTime (seconds, time, sizeof (time));
					std::snprintf (buf, sizeof (buf), "%s: %s", def.title, time);
				}
				break;
			}

			// Pan reads as the offset it is, not as a position.
			if (tag == kPan)
			{
				char where[12] = {};
				formatPan (inner, where, sizeof (where));
				std::snprintf (buf, sizeof (buf), "%s: %s", def.title, where);
				break;
			}

			// The ladder switch says what it does rather than On/Off: the
			// bypass is level-matched, so "out" is not "quieter".
			if (tag == kLadderEnable)
			{
				std::snprintf (buf, sizeof (buf), "%s: %s", def.title,
				               (inner >= 0.5) ? "in" : "bypassed");
				break;
			}

			// The output trim reads in decibels, which is the only unit a
			// level control has any business showing.
			if (tag == kOutputLevel)
			{
				if (inner <= 0.0)
					std::snprintf (buf, sizeof (buf), "%s: off", def.title);
				else
					std::snprintf (buf, sizeof (buf), "%s: %+.1f dB", def.title,
					               outputDecibels (inner));
				break;
			}

			// The LFO rate knobs read in hertz. harmonicLfoHz is the same
			// function Voice sets its oscillators from, so the panel cannot
			// be showing a rate the sound is not running at.
			if (tag >= kHarmLfoRate1 && tag <= kHarmLfoRate10)
			{
				const double hz = harmonicLfoHz (inner);
				if (hz < 0.995)
					std::snprintf (buf, sizeof (buf), "%s: %.3f Hz", def.title, hz);
				else
					std::snprintf (buf, sizeof (buf), "%s: %.2f Hz", def.title, hz);
				break;
			}

			// Depth zero is not "0 %", it is the LFO switched off - and off
			// is what nine patches in ten want, so it is worth naming.
			if (tag >= kHarmLfoDepth1 && tag <= kHarmLfoDepth10 && plain <= 0.0)
			{
				std::snprintf (buf, sizeof (buf), "%s: off", def.title);
				break;
			}

			// The ten phase controls read in degrees. The knob is a real
			// -90..+90 phase shift (see ForTranParams.h); a percentage says
			// nothing about a phase, and reading "50 %" at the centre is
			// what kept the control from being recognised for what it is.
			if (isPhaseParam (tag))
			{
				char deg[8] = {};
				formatPhase (phaseDegrees (inner), deg, sizeof (deg));
				// U+00B0 as UTF-8; the readout draws a VSTGUI UTF8String.
				std::snprintf (buf, sizeof (buf), "%s: %s\xc2\xb0", def.title, deg);
				break;
			}

			// A harmonic at zero reads "muted" rather than "0 %", so the
			// state alt-click puts it in has a name on screen. See
			// FtVSlider::setMuteable.
			if (tag >= kHarm1 && tag <= kHarm10 && plain <= 0.0)
			{
				std::snprintf (buf, sizeof (buf), "%s: muted", def.title);
				break;
			}

			std::snprintf (buf, sizeof (buf), "%s: %.0f%%", def.title, plain);
			break;
	}

	return buf;
}

//------------------------------------------------------------------------
void ForTranEditor::showReadoutFor (CControl* control)
{
	if (!mReadout || !control || !frame)
		return;

	const ParamID tag = static_cast<ParamID> (control->getTag ());
	if (tag >= kNumParams)               // the formant selector buttons
		return;

	const std::string text = readoutTextFor (tag);
	if (text.empty ())
		return;
	mReadout->setText (text);

	// Below the control, or above it when there is no room below. The view
	// spans the whole window width and the plate is centred on the control
	// within it, so a control at the very edge gets its reading alongside
	// rather than shunted into the middle.
	// A view's size is in its PARENT's coordinates. Most controls sit
	// directly on the frame so the two agree, but the twenty LFO knobs live
	// inside a container, and using their local rect would put the reading
	// a couple of hundred pixels away from the knob.
	CRect c = control->getViewSize ();
	for (CView* p = control->getParentView (); p != nullptr; p = p->getParentView ())
	{
		if (p == frame)
			break;
		c.offset (p->getViewSize ().left, p->getViewSize ().top);
	}

	const CCoord h = 15.;
	CCoord y = c.bottom + 2.;
	if (y + h > kEditorHeight)
		y = c.top - h - 2.;
	y = std::max<CCoord> (0., std::min<CCoord> (y, kEditorHeight - h));

	const CRect r (0., y, static_cast<CCoord> (kEditorWidth), y + h);
	mReadout->setViewSize (r);
	mReadout->setMouseableArea (r);
	mReadout->setAnchorX (c.left + c.getWidth () / 2.);
	mReadout->setVisible (true);
	mReadout->invalid ();
}

//------------------------------------------------------------------------
void ForTranEditor::hideReadout ()
{
	if (mReadout && mReadout->isVisible ())
	{
		mReadout->setVisible (false);
		mReadout->invalid ();
	}
}

//------------------------------------------------------------------------
void ForTranEditor::setLiveWaveform (const float* samples, int count)
{
	if (mWaveView && samples && count > 1)
		mWaveView->setData (samples, count, true);
}

//------------------------------------------------------------------------
void ForTranEditor::controlHovered (CControl* control, bool entered)
{
	if (entered)
	{
		mHovered = control;
		showReadoutFor (control);
	}
	else
	{
		if (mHovered == control)
			mHovered = nullptr;
		// Keep it up while a drag is in progress: the pointer routinely
		// leaves the control it is dragging.
		if (!mEditing)
			hideReadout ();
	}
}

//------------------------------------------------------------------------
void ForTranEditor::controlBeginEdit (CControl* control)
{
	mEditing = control;
	showReadoutFor (control);
}

//------------------------------------------------------------------------
void ForTranEditor::controlEndEdit (CControl* control)
{
	if (mEditing == control)
		mEditing = nullptr;
	if (!mHovered)
		hideReadout ();
}

//------------------------------------------------------------------------
bool ForTranEditor::affectsDisplays (ParamID tag)
{
	if (tag == kCutoff || tag == kFilterGain || tag == kPoles)
		return true;
	if (tag == kLowPassState || tag == kHighPassState ||
	    tag == kCombState || tag == kGrassState)
		return true;
	if (tag >= kHarm1 && tag <= kHarm10)             return true;
	if (tag >= kHarmPhase1 && tag <= kHarmPhase10)   return true;
	if (tag >= kHarmInv1 && tag <= kHarmInv10)       return true;
	return false;
}

//------------------------------------------------------------------------
// Both panels are computed from the parameters, not captured from the DSP.
//
// The DXi captured the waveform: synthNote wrote one cycle into pfWaveShape
// on a zero crossing and the property page pulled it back through MyProps.
// VST3 has no such back-channel, and does not need one here - ForTran is
// additive, so one cycle is fully determined by the harmonic levels, the
// phases and the filter. Recomputing is cheaper than plumbing a ring buffer
// across the component boundary, and it cannot go stale.
//------------------------------------------------------------------------
void ForTranEditor::refreshDisplays ()
{
	if (!mController)
		return;

	auto internalOf = [this] (ParamID id) {
		return paramDef (id).toInternal (mController->getParamNormalized (id));
	};

	FilterInfo::ShapeStates states;
	states.lowPass  = static_cast<int> (internalOf (kLowPassState));
	states.highPass = static_cast<int> (internalOf (kHighPassState));
	states.comb     = static_cast<int> (internalOf (kCombState));
	states.grass    = static_cast<int> (internalOf (kGrassState));

	mFilter.setCutoff (static_cast<float> (internalOf (kCutoff)));
	mFilter.setFeedback (static_cast<float> (internalOf (kFilterGain)) * 3.9f);
	mFilter.setPoles (1.f + static_cast<float> (internalOf (kPoles)) * 10.f);
	if (mFilter.needsRecalculation ())
		mFilter.calculate ();

	if (mResponseView)
	{
		float curve[FilterInfo::kNumBins];
		mFilter.combinedCurve (states, curve);
		mResponseView->setData (curve, FilterInfo::kNumBins, false);
	}

	if (mWaveView)
	{
		// The filter is sampled at bin 276 - 440 Hz, where the filter
		// envelope starts before it sweeps.
		float magnitude[FilterInfo::kNumParts];
		float theta[FilterInfo::kNumParts];
		mFilter.combine (states, 276, magnitude, theta);

		float level[10], phase[10];
		for (int k = 0; k < 10; ++k)
		{
			const double lv  = internalOf (static_cast<ParamID> (kHarm1 + k));
			const bool   inv = internalOf (static_cast<ParamID> (kHarmInv1 + k)) >= 0.5;
			level[k] = static_cast<float> (inv ? -lv : lv);
			// dPIm * (0.5 - control), with the DXi's negative pi.
			phase[k] = static_cast<float> (
			    -3.14159265358979323846 *
			    (0.5 - internalOf (static_cast<ParamID> (kHarmPhase1 + k))));
		}

		constexpr int kPoints = 256;
		float cycle[kPoints];
		for (int i = 0; i < kPoints; ++i)
		{
			const double t = 2.0 * 3.14159265358979323846 * i / (kPoints - 1);
			double sum = 0.0;
			for (int k = 0; k < 10; ++k)
				sum += level[k] * std::sin ((k + 1) * t + theta[k] + phase[k]) * magnitude[k];
			cycle[i] = static_cast<float> (sum);
		}
		mWaveView->setData (cycle, kPoints, true);
	}
}

//------------------------------------------------------------------------
bool ForTranEditor::affectsEnvelopes (ParamID tag)
{
	if (tag >= kEnvAttack && tag <= kEnvLevel)             return true;
	if (tag >= kFilterAttack && tag <= kFilterLevel)       return true;
	if (tag >= kFormantAttack1 && tag <= kFormantLevel10)  return true;
	if (tag >= kFormantEnv1 && tag <= kFormantEnv10)       return true;
	if (tag >= kFormantInv1 && tag <= kFormantInv10)       return true;
	return false;
}

//------------------------------------------------------------------------
EnvelopeSpec ForTranEditor::envelopeSpec (int which) const
{
	EnvelopeSpec spec;
	if (!mController)
		return spec;

	auto internalOf = [this] (ParamID id) {
		return static_cast<float> (paramDef (id).toInternal (mController->getParamNormalized (id)));
	};

	// The ids are read in the same groupings Voice::noteOn uses, so the two
	// cannot drift: five consecutive parameters per envelope, then the two
	// flags out of their own blocks for the formants.
	if (which == 0)
	{
		spec.attack  = internalOf (kEnvAttack);
		spec.decay   = internalOf (kEnvDecay);
		spec.sustain = internalOf (kEnvSustain);
		spec.release = internalOf (kEnvRelease);
		spec.level   = internalOf (kEnvLevel);
		spec.enabled = true;                 // the DXi always enabled this one
		spec.invert  = false;
	}
	else if (which == 1)
	{
		spec.attack  = internalOf (kFilterAttack);
		spec.decay   = internalOf (kFilterDecay);
		spec.sustain = internalOf (kFilterSustain);
		spec.release = internalOf (kFilterRelease);
		spec.level   = internalOf (kFilterLevel);
		spec.enabled = true;
		spec.invert  = false;
	}
	else
	{
		const int n = std::clamp (mSelectedFormant, 0, 9);
		const ParamID base = static_cast<ParamID> (kFormantAttack1 + n * 5);
		spec.attack  = internalOf (base);
		spec.decay   = internalOf (static_cast<ParamID> (base + 1));
		spec.sustain = internalOf (static_cast<ParamID> (base + 2));
		spec.release = internalOf (static_cast<ParamID> (base + 3));
		spec.level   = internalOf (static_cast<ParamID> (base + 4));
		spec.enabled = internalOf (static_cast<ParamID> (kFormantEnv1 + n)) >= 0.5f;
		spec.invert  = internalOf (static_cast<ParamID> (kFormantInv1 + n)) >= 0.5f;
	}
	return spec;
}

//------------------------------------------------------------------------
// The three envelope panels.
//
// traceEnvelope drives the real Adsr, so these show what the voices will do
// rather than an idealised trapezoid - the sustain sitting just below its
// control, an inverted formant upside down, a disabled one flat. Each panel
// scales its own time axis to what it is showing and prints the span, so a
// short envelope is still readable and the number says how long it is.
//------------------------------------------------------------------------
void ForTranEditor::refreshEnvelopes ()
{
	constexpr int kPoints = 220;
	static const char* const kNames[3] = { "Amp Env", "Filter Env", "Harmonic" };

	for (int i = 0; i < 3; ++i)
	{
		if (!mEnvViews[i])
			continue;

		const EnvelopeSpec spec = envelopeSpec (i);
		float shape[kPoints];
		const double seconds = traceEnvelope (spec, shape, kPoints);

		char caption[32];
		if (i == 2)
			std::snprintf (caption, sizeof (caption), "Harmonic %d", mSelectedFormant + 1);
		else
			std::snprintf (caption, sizeof (caption), "%s", kNames[i]);

		char annotation[24] = {};
		if (!spec.enabled)
			std::snprintf (annotation, sizeof (annotation), "off");
		else
			formatEnvelopeTime (seconds, annotation, sizeof (annotation));

		mEnvViews[i]->setCaption (caption);
		mEnvViews[i]->setAnnotation (annotation);
		mEnvViews[i]->setData (shape, kPoints, false);
	}
}

//------------------------------------------------------------------------
// The harmonic LFO panel
//
// Twenty knobs have nowhere to go on a panel that already has no room for a
// permanent value readout, so they live on a panel that is summoned. It is a
// child of the frame rather than a platform window: a real window would have
// to be parented, positioned, kept above the editor and torn down by hand on
// every host's terms, while a child view cannot be misplaced, cannot outlive
// the editor, and draws in the same coordinates as everything else.
//
// Three ways out, because a panel that traps you is worse than no panel:
// the Close button, a click anywhere outside it, and Escape.
//------------------------------------------------------------------------
void ForTranEditor::buildLfoPanel ()
{
	if (!frame)
		return;

	// The sheet goes on first so it sits UNDER the panel and over everything
	// else: it closes on a click and, while it is up, nothing behind it can
	// be dragged by accident.
	mLfoScrim = new FtScrim (CRect (0, 0, kEditorWidth, kEditorHeight),
	                         [this] () { showLfoPanel (false); });
	mLfoScrim->setVisible (false);
	frame->addView (mLfoScrim);

	const CCoord left = (kEditorWidth - kLfoPanelWidth) / 2.;
	const CCoord top  = (kEditorHeight - kLfoPanelHeight) / 2.;
	auto* panel = new FtPanel (CRect (left, top, left + kLfoPanelWidth,
	                                  top + kLfoPanelHeight),
	                           "Harmonic LFO");
	panel->setVisible (false);
	mLfoPanel = panel;

	// The grip goes on FIRST, so it is the bottom-most child: hit testing
	// runs topmost-first, and the Close button that overlaps it is added
	// after and therefore wins where the two meet.
	panel->addView (new FtPanelGrip (CRect (0, 0, kLfoPanelWidth, FtPanel::kTitleHeight),
	                                 panel));

	panel->addView (new FtTextButton (CRect (kLfoPanelWidth - 52, 4,
	                                         kLfoPanelWidth - 8, 18),
	                                  "Close", [this] () { showLfoPanel (false); }));

	// Column numbers, then a Rate row and a Depth row. The columns are on
	// the harmonic sliders' own 30 px pitch, so column n here is under
	// harmonic n there.
	const CCoord numbersY = 24., rateY = 36., depthY = 88.;
	for (int n = 0; n < 10; ++n)
	{
		const CCoord x = kLfoColumn0 + n * kLfoColumnPitch;
		char number[4];
		std::snprintf (number, sizeof (number), "%d", n + 1);
		panel->addView (new FtLabel (CRect (x, numbersY, x + kKnobSize, numbersY + 11),
		                             number));
		addKnobAt (static_cast<ParamID> (kHarmLfoRate1 + n),
		           CRect (x, rateY, x + kKnobSize, rateY + kKnobSize), panel);
		addKnobAt (static_cast<ParamID> (kHarmLfoDepth1 + n),
		           CRect (x, depthY, x + kKnobSize, depthY + kKnobSize), panel);
	}

	panel->addView (new FtLabel (CRect (8, rateY + 9, kLfoColumn0 - 6, rateY + 21),
	                             "Rate", kRightText));
	panel->addView (new FtLabel (CRect (8, depthY + 9, kLfoColumn0 - 6, depthY + 21),
	                             "Depth", kRightText));
	panel->addView (new FtLabel (CRect (8, kLfoPanelHeight - 18,
	                                    kLfoPanelWidth - 8, kLfoPanelHeight - 6),
	                             "Depth 0 is off  -  drag the title bar to move  -  Esc closes",
	                             kCenterText, CColor (255, 255, 255, 120)));

	frame->addView (panel);
}

//------------------------------------------------------------------------
void ForTranEditor::showLfoPanel (bool show)
{
	if (mLfoScrim)
	{
		mLfoScrim->setVisible (show);
		mLfoScrim->invalid ();
	}
	if (mLfoPanel)
	{
		mLfoPanel->setVisible (show);
		mLfoPanel->invalid ();
	}
	// The readout follows the pointer, and the pointer is about to be
	// somewhere else entirely.
	hideReadout ();
	if (frame)
		frame->invalid ();
}

//------------------------------------------------------------------------
void ForTranEditor::onKeyboardEvent (KeyboardEvent& event, CFrame*)
{
	if (event.type != EventType::KeyDown || event.virt != VirtualKey::Escape)
		return;
	if (mLfoPanel && mLfoPanel->isVisible ())
	{
		showLfoPanel (false);
		event.consumed = true;
	}
}

//------------------------------------------------------------------------
void PLUGIN_API ForTranEditor::close ()
{
	mControls.clear ();
	for (auto& c : mFormantControls) c = nullptr;
	for (auto& b : mFormantButtons)  b = nullptr;
	mResponseView = nullptr;
	mWaveView = nullptr;
	for (auto& v : mEnvViews) v = nullptr;
	mReadout = nullptr;
	mHovered = nullptr;
	mEditing = nullptr;
	mScopeTimer = nullptr;          // stopped before the frame goes away
	mLfoPanel = nullptr;            // owned by the frame, released with it
	mLfoScrim = nullptr;

	if (frame)
	{
		frame->unregisterKeyboardHook (this);
		frame->forget ();
		frame = nullptr;
	}
	mBitmaps.clear ();
}

//------------------------------------------------------------------------
void ForTranEditor::valueChanged (CControl* control)
{
	if (!control || !mController || mUpdating)
		return;

	const int32_t tag = control->getTag ();

	// 2000..2009 are the formant selector buttons - editor state, not
	// parameters, so they never reach the host.
	if (tag >= 2000 && tag <= 2009)
	{
		selectFormant (tag - 2000);
		return;
	}

	const ParamID    id    = static_cast<ParamID> (tag);
	const ParamValue value = control->getValueNormalized ();

	mController->setParamNormalized (id, value);
	mController->performEdit (id, value);

	if (affectsDisplays (id))
		refreshDisplays ();
	if (affectsEnvelopes (id))
		refreshEnvelopes ();

	if (mReadout && mReadout->isVisible () && (control == mEditing || control == mHovered))
		mReadout->setText (readoutTextFor (id));
}

//------------------------------------------------------------------------
void ForTranEditor::updateControl (ParamID tag, ParamValue normalized)
{
	auto it = mControls.find (tag);
	if (it != mControls.end () && it->second)
	{
		mUpdating = true;
		it->second->setValueNormalized (static_cast<float> (normalized));
		it->second->invalid ();
		mUpdating = false;
	}

	if (affectsDisplays (tag))
		refreshDisplays ();
	if (affectsEnvelopes (tag))
		refreshEnvelopes ();

	// The formant knobs are not in mControls - they are re-pointed - so
	// catch a change to whichever formant is on display.
	for (auto* c : mFormantControls)
		if (c && static_cast<ParamID> (c->getTag ()) == tag)
		{
			mUpdating = true;
			c->setValueNormalized (static_cast<float> (normalized));
			c->invalid ();
			mUpdating = false;
		}
}

//------------------------------------------------------------------------
} // namespace ForTran
