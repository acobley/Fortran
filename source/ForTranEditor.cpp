//------------------------------------------------------------------------
// ForTran - editor implementation: the Polivoks panel
//
// Every coordinate below is the one tools/polivoks/d2p.js draws at. The
// panel bitmap and the wear layer are rendered from that file, so a control
// moved here and not there will sit on the wrong printed scale.
//------------------------------------------------------------------------

#include "ForTranEditor.h"
#include "Voice.h"
#include "ForTranController.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>

using namespace VSTGUI;
using namespace Steinberg;
using namespace Steinberg::Vst;

namespace ForTran {

namespace {

/** The ten partial columns in the bank, and the bars over them. */
inline double bankX (int i) { return 78. + i * 44.8; }

/** Tags 2000..2009 are the partial selector rings - editor state. */
constexpr int32_t kBadgeTag0 = 2000;

std::string upper (std::string s)
{
	for (auto& c : s)
		c = static_cast<char> (std::toupper (static_cast<unsigned char> (c)));
	return s;
}

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
/** The panel's bitmaps are drawn at 2x, so they are sharp on a Retina
    screen; declaring the scale makes VSTGUI draw them at 1200 x 640. */
CBitmap* ForTranEditor::bitmap2x (const char* name)
{
	auto* bmp = new CBitmap (CResourceDescription (name));
	if (auto pb = bmp->getPlatformBitmap ())
		pb->setScaleFactor (2.);
	return bmp;
}

//------------------------------------------------------------------------
double ForTranEditor::internalOf (ParamID id) const
{
	if (!mController || id >= kNumParams)
		return 0.;
	return paramDef (id).toInternal (mController->getParamNormalized (id));
}

//------------------------------------------------------------------------
void ForTranEditor::setDefault (CControl* control, ParamID tag)
{
	if (control && tag < kNumParams)
		control->setDefaultValue (static_cast<float> (paramDef (tag).defaultNormalized ()));
}

//------------------------------------------------------------------------
void ForTranEditor::registerControl (ParamID tag, CControl* control)
{
	mControls[tag].push_back (control);
	if (mController)
		control->setValueNormalized (static_cast<float> (mController->getParamNormalized (tag)));
	setDefault (control, tag);
	frame->addView (control);
}

//------------------------------------------------------------------------
PvKnob* ForTranEditor::addKnob (ParamID tag, double cx, double cy, double outer)
{
	auto* k = new PvKnob (CPoint (cx, cy), outer, this, static_cast<int32_t> (tag));
	k->setHoverListener (this);
	registerControl (tag, k);
	return k;
}

//------------------------------------------------------------------------
void ForTranEditor::addValue (CControl* control, double x, double y, double size,
                              SciFi::Align align, CColor colour)
{
	auto* t = new PvText (CPoint (x, y), size, false, align, colour, 90.);
	frame->addView (t);
	mValues.push_back ({ control, t });
}

//------------------------------------------------------------------------
PvSelector* ForTranEditor::addSelector (ParamID tag, double cx, double cy, double r,
                                        std::vector<std::string> names, double legendSize,
                                        double legendRadius)
{
	auto* s = new PvSelector (CPoint (cx, cy), r, std::move (names), legendSize, legendRadius,
	                          this, static_cast<int32_t> (tag));
	s->setHoverListener (this);
	registerControl (tag, s);
	return s;
}

//------------------------------------------------------------------------
PvLamp* ForTranEditor::addLamp (ParamID tag, double cx, double cy)
{
	auto* l = new PvLamp (CPoint (cx, cy), this, static_cast<int32_t> (tag));
	l->setHoverListener (this);
	registerControl (tag, l);
	return l;
}

//------------------------------------------------------------------------
bool PLUGIN_API ForTranEditor::open (void* parent, const PlatformType& platformType)
{
	if (frame)
		return false;

	CRect frameSize (0, 0, kEditorWidth, kEditorHeight);
	frame = new CFrame (frameSize, this);
	frame->setBackgroundColor (Pv::kPanel);
	frame->open (parent, platformType);

	mPanelBitmap = VSTGUI::owned (bitmap2x ("pv_panel.png"));
	mWearBitmap  = VSTGUI::owned (bitmap2x ("pv_wear.png"));
	if (mPanelBitmap)
		frame->setBackground (mPanelBitmap);

	const std::vector<std::string> offOn = { "OFF", "ON" };
	const std::vector<std::string> fourWay = { "OFF", "ADD", "MULT", "MAX" };

	// ---- header: the engine line and the voice meter --------------------
	mEngineText = new PvText (CPoint (252., 32.), 10.5, false, SciFi::Align::Left, Pv::kInk, 420.);
	frame->addView (mEngineText);
	mVoiceMeter = new PvVoiceMeter (CRect (970., 19., 1178., 37.));
	frame->addView (mVoiceMeter);

	// ---- PARTIALS: the bar graph and the bank ---------------------------
	mBars = new PvBarsOled (CRect (30., 106., 526., 214.));
	frame->addView (mBars);
	for (int i = 0; i < 10; ++i)
	{
		const double x = bankX (i);
		mBadges[i] = new PvBadge (CPoint (x, 238.), i + 1, this, kBadgeTag0 + i);
		frame->addView (mBadges[i]);
		addKnob (static_cast<ParamID> (kHarm1 + i), x, 271., 14.);
		addKnob (static_cast<ParamID> (kHarmPhase1 + i), x, 309., 10.);
		addLamp (static_cast<ParamID> (kHarmInv1 + i), x, 339.);
		addLamp (static_cast<ParamID> (kFormantEnv1 + i), x, 362.);
	}
	mCopyButton = new PvButton (CRect (406., 392., 526., 410.), "COPY 1 TO ALL", 8.5,
	                            [this] () { copyPartialToAll (); });
	frame->addView (mCopyButton);

	// ---- SPECTRAL FILTER --------------------------------------------------
	addLamp (kBypass, 844., 94.);
	mFilterOled = new PvFilterOled (CRect (570., 106., 882., 214.));
	frame->addView (mFilterOled);
	addSelector (kLowPassState,  604., 258., 11., offOn,   8.5, 22.);
	addSelector (kHighPassState, 682., 258., 11., fourWay, 7.5, 20.);
	addSelector (kCombState,     760., 258., 11., fourWay, 7.5, 20.);
	addSelector (kGrassState,    840., 258., 11., fourWay, 7.5, 20.);
	{
		const ParamID tags[4] = { kCutoff, kFilterGain, kPoles, kFilterLevel };
		const double xs[4] = { 604., 682., 760., 840. };
		for (int i = 0; i < 4; ++i)
			addValue (addKnob (tags[i], xs[i], 330., 18.), xs[i], 359., 9.5);
	}
	for (int i = 0; i < 4; ++i)
	{
		const double x = 640. + i * 66.;
		addValue (addKnob (static_cast<ParamID> (kFilterAttack + i), x, 408., 9.),
		          x + 14., 417., 7.5, SciFi::Align::Left, Pv::kMuted);
	}

	// ---- AMP ----------------------------------------------------------------
	mAmpOled = new PvEnvOled (CRect (926., 106., 1170., 214.), Pv::kMint, true);
	frame->addView (mAmpOled);
	{
		const ParamID tags[5] = { kEnvAttack, kEnvDecay, kEnvSustain, kEnvRelease, kEnvLevel };
		for (int i = 0; i < 5; ++i)
		{
			const double x = 938. + i * 49.;
			addValue (addKnob (tags[i], x, 250., 15.), x, 276., 8.5);
		}
	}
	addValue (addKnob (kOutputLevel, 954., 336., 17.), 954., 364., 9.5);
	addValue (addKnob (kPan, 1020., 336., 17.), 1020., 364., 9.5);
	addValue (addKnob (kPortamento, 1086., 336., 17.), 1086., 364., 9.5);
	addSelector (kLadderEnable, 1146., 336., 10., offOn, 7.5, 21.);

	// ---- PARTIAL n: one box, re-pointed at the selected partial -------------
	{
		const double tx = 32., size = 13.;
		const double numberX = tx + 5. + SciFi::textWidth ("PARTIAL", size, true) + 4.4 * size * 0.64 / 6.;
		mPartialNumber = new PvText (CPoint (numberX, 474.5), size, true, SciFi::Align::Left, Pv::kInk, 30.);
		frame->addView (mPartialNumber);
	}
	mPrevButton = new PvButton (CRect (440., 459., 480., 477.), "", 9.,
	                            [this] () { selectPartial ((mSelected + 9) % 10); });
	mNextButton = new PvButton (CRect (486., 459., 526., 477.), "", 9.,
	                            [this] () { selectPartial ((mSelected + 1) % 10); });
	frame->addView (mPrevButton);
	frame->addView (mNextButton);
	mPartialOled = new PvEnvOled (CRect (30., 488., 202., 610.), Pv::kAmber, true);
	frame->addView (mPartialOled);
	{
		auto knob = [this] (int slot, double cx, double cy, double outer) {
			auto* k = new PvKnob (CPoint (cx, cy), outer, this, 0);
			k->setHoverListener (this);
			mPartialControls[slot] = k;
			frame->addView (k);
			return k;
		};
		const double xs[4] = { 236., 284., 332., 380. };
		for (int i = 0; i < 4; ++i)
			addValue (knob (kPA + i, xs[i], 512., 14.), xs[i], 537., 8.5);
		addValue (knob (kPLevel, 440., 512., 18.), 440., 541., 9.5);

		auto* env = new PvSelector (CPoint (498., 512.), 10., offOn, 7.5, 21., this, 0);
		env->setHoverListener (this);
		mPartialControls[kPEnv] = env;
		frame->addView (env);

		addValue (knob (kPRate, 316., 586., 11.), 332., 596., 8., SciFi::Align::Left, Pv::kMuted);
		addValue (knob (kPDepth, 410., 586., 11.), 426., 596., 8., SciFi::Align::Left, Pv::kMuted);

		auto* inv = new PvLamp (CPoint (516., 598.), this, 0);
		inv->setHoverListener (this);
		mPartialControls[kPInv] = inv;
		frame->addView (inv);
	}

	// ---- MODULATION --------------------------------------------------------
	{
		const ParamID tags[4] = { kVibratoRate, kVibratoDepth, kTremoloRate, kTremoloDepth };
		const double xs[4] = { 594., 656., 734., 796. };
		for (int i = 0; i < 4; ++i)
			addValue (addKnob (tags[i], xs[i], 540., 18.), xs[i], 569., 9.5);
	}
	mRings = new PvRingsOled (CRect (862., 488., 1170., 610.), [this] (int i) { selectPartial (i); });
	frame->addView (mRings);

	// ---- the wear, then the readout over everything ------------------------
	if (mWearBitmap)
		frame->addView (new PvOverlay (frameSize, mWearBitmap));
	mReadout = new PvReadout (CRect (0, 0, kEditorWidth, 16));
	mReadout->setVisible (false);
	frame->addView (mReadout);

	selectPartial (0);
	refreshDisplays ();
	refreshValues ();
	refreshHeader ();

	mTimer = makeOwned<CVSTGUITimer> (
	    [this] (CVSTGUITimer*) {
		    if (mController)
			    mController->requestWaveFrame ();
		    refreshHeader ();
	    }, 50, true);

	return true;
}

//------------------------------------------------------------------------
void ForTranEditor::selectPartial (int index)
{
	mSelected = std::clamp (index, 0, 9);
	for (int i = 0; i < 10; ++i)
		if (mBadges[i])
		{
			mBadges[i]->setValueNormalized (i == mSelected ? 1.f : 0.f);
			mBadges[i]->invalid ();
		}
	const int n = mSelected + 1;
	if (mPartialNumber) mPartialNumber->setText (std::to_string (n));
	if (mPrevButton)    mPrevButton->setText ("\xe2\x80\xb9 " + std::to_string ((mSelected + 9) % 10 + 1));
	if (mNextButton)    mNextButton->setText (std::to_string ((mSelected + 1) % 10 + 1) + " \xe2\x80\xba");
	if (mCopyButton)    mCopyButton->setText ("COPY " + std::to_string (n) + " TO ALL");

	retargetPartialControls ();
	refreshDisplays ();
	refreshEnvelopes ();
	refreshLfos ();
	refreshValues ();
}

//------------------------------------------------------------------------
void ForTranEditor::retargetPartialControls ()
{
	const int i = mSelected;
	const ParamID base = static_cast<ParamID> (kFormantAttack1 + i * 5);
	const ParamID tags[kNumPartialControls] = {
		base, static_cast<ParamID> (base + 1), static_cast<ParamID> (base + 2),
		static_cast<ParamID> (base + 3), static_cast<ParamID> (base + 4),
		static_cast<ParamID> (kFormantEnv1 + i), static_cast<ParamID> (kFormantInv1 + i),
		static_cast<ParamID> (kHarmLfoRate1 + i), static_cast<ParamID> (kHarmLfoDepth1 + i),
	};

	mUpdating = true;
	for (int k = 0; k < kNumPartialControls; ++k)
	{
		auto* c = mPartialControls[k];
		if (!c)
			continue;
		c->setTag (static_cast<int32_t> (tags[k]));
		setDefault (c, tags[k]);
		if (mController)
			c->setValueNormalized (static_cast<float> (mController->getParamNormalized (tags[k])));
		c->invalid ();
	}
	mUpdating = false;
}

//------------------------------------------------------------------------
// Copy the selected partial's box - its envelope, envelope switches and
// harmonic LFO - to the other nine. Level, phase and invert stay put: they
// are what makes the partials different from one another.
//------------------------------------------------------------------------
void ForTranEditor::copyPartialToAll ()
{
	if (!mController)
		return;
	const int from = mSelected;
	auto copy = [this] (ParamID src, ParamID dst) {
		const ParamValue v = mController->getParamNormalized (src);
		mController->beginEdit (dst);
		mController->setParamNormalized (dst, v);
		mController->performEdit (dst, v);
		mController->endEdit (dst);
		updateControl (dst, v);
	};
	for (int to = 0; to < 10; ++to)
	{
		if (to == from)
			continue;
		for (int k = 0; k < 5; ++k)
			copy (static_cast<ParamID> (kFormantAttack1 + from * 5 + k),
			      static_cast<ParamID> (kFormantAttack1 + to * 5 + k));
		copy (static_cast<ParamID> (kFormantEnv1 + from),   static_cast<ParamID> (kFormantEnv1 + to));
		copy (static_cast<ParamID> (kFormantInv1 + from),   static_cast<ParamID> (kFormantInv1 + to));
		copy (static_cast<ParamID> (kHarmLfoRate1 + from),  static_cast<ParamID> (kHarmLfoRate1 + to));
		copy (static_cast<ParamID> (kHarmLfoDepth1 + from), static_cast<ParamID> (kHarmLfoDepth1 + to));
	}
}

//------------------------------------------------------------------------
// The readout: the name and value of whatever is under the pointer.
//------------------------------------------------------------------------
std::string ForTranEditor::readoutTextFor (ParamID tag) const
{
	if (!mController)
		return {};
	if (tag == kBypass)
		return mController->getParamNormalized (kBypass) >= 0.5 ? "Bypass: on" : "Bypass: off";
	if (tag >= kNumParams)
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

			// Envelope times read as a time: Adsr squares the control, so
			// a percentage would say almost nothing.
			if (const double seconds = envelopeTimeSeconds (tag, inner); seconds >= 0.0)
			{
				char time[16] = {};
				formatEnvelopeTime (seconds, time, sizeof (time));
				std::snprintf (buf, sizeof (buf), "%s: %s", def.title, time);
				break;
			}

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

			if (tag == kPan)
			{
				char where[12] = {};
				formatPan (inner, where, sizeof (where));
				std::snprintf (buf, sizeof (buf), "%s: %s", def.title, where);
				break;
			}

			// The ladder's bypass is level-matched, so "out" is not "quieter".
			if (tag == kLadderEnable)
			{
				std::snprintf (buf, sizeof (buf), "%s: %s", def.title,
				               (inner >= 0.5) ? "in" : "bypassed");
				break;
			}

			if (tag == kOutputLevel)
			{
				if (inner <= 0.0)
					std::snprintf (buf, sizeof (buf), "%s: off", def.title);
				else
					std::snprintf (buf, sizeof (buf), "%s: %+.1f dB", def.title,
					               outputDecibels (inner));
				break;
			}

			// harmonicLfoHz is the function Voice sets its oscillators from.
			if (tag >= kHarmLfoRate1 && tag <= kHarmLfoRate10)
			{
				const double hz = harmonicLfoHz (inner);
				if (hz < 0.995)
					std::snprintf (buf, sizeof (buf), "%s: %.3f Hz", def.title, hz);
				else
					std::snprintf (buf, sizeof (buf), "%s: %.2f Hz", def.title, hz);
				break;
			}

			if (tag >= kHarmLfoDepth1 && tag <= kHarmLfoDepth10 && plain <= 0.0)
			{
				std::snprintf (buf, sizeof (buf), "%s: off", def.title);
				break;
			}

			// A real -90..+90 phase shift, so degrees.
			if (isPhaseParam (tag))
			{
				char deg[8] = {};
				formatPhase (phaseDegrees (inner), deg, sizeof (deg));
				std::snprintf (buf, sizeof (buf), "%s: %s deg", def.title, deg);
				break;
			}

			if (tag >= kHarm1 && tag <= kHarm10 && plain <= 0.0)
			{
				std::snprintf (buf, sizeof (buf), "%s: muted", def.title);
				break;
			}

			std::snprintf (buf, sizeof (buf), "%s: %.0f %%", def.title, plain);
			break;
	}

	return buf;
}

//------------------------------------------------------------------------
std::string ForTranEditor::valueTextFor (ParamID tag) const
{
	if (!mController || tag >= kNumParams)
		return {};

	if (tag == kCutoff)
	{
		const int bin = std::clamp (static_cast<int> (internalOf (kCutoff) * FilterInfo::kNumBins),
		                            0, FilterInfo::kNumBins - 1);
		const double hz = mFilter.binFrequency (bin);
		char buf[16];
		if (hz < 1000.)
			std::snprintf (buf, sizeof (buf), "%.0f HZ", hz);
		else
			std::snprintf (buf, sizeof (buf), "%.2fK", hz / 1000.);
		return buf;
	}
	if (tag == kOutputLevel)
	{
		const double inner = internalOf (kOutputLevel);
		if (inner <= 0.)
			return "OFF";
		char buf[16];
		std::snprintf (buf, sizeof (buf), "%+.0f DB", outputDecibels (inner));
		return buf;
	}
	if (tag >= kHarmLfoRate1 && tag <= kHarmLfoRate10)
	{
		const double hz = harmonicLfoHz (internalOf (tag));
		char buf[16];
		std::snprintf (buf, sizeof (buf), hz < 10. ? "%.2f HZ" : "%.1f HZ", hz);
		return buf;
	}

	std::string s = readoutTextFor (tag);
	const auto colon = s.find (": ");
	if (colon != std::string::npos)
		s = s.substr (colon + 2);
	return upper (s);
}

//------------------------------------------------------------------------
void ForTranEditor::refreshValues ()
{
	for (auto& v : mValues)
		if (v.control && v.text)
			v.text->setText (valueTextFor (static_cast<ParamID> (v.control->getTag ())));
}

//------------------------------------------------------------------------
void ForTranEditor::refreshHeader ()
{
	if (!mEngineText || !mController)
		return;
	char buf[80];
	std::snprintf (buf, sizeof (buf), "%.0f HZ  /  16 VOICES  /  PITCH WHEEL 1 OCT",
	               mController->getHostSampleRate ());
	mEngineText->setText (buf);
}

//------------------------------------------------------------------------
void ForTranEditor::showReadoutFor (CControl* control)
{
	if (!mReadout || !control || !frame)
		return;
	const std::string text = upper (readoutTextFor (static_cast<ParamID> (control->getTag ())));
	if (text.empty ())
		return;
	mReadout->setText (text);

	// Below the control, or above it when there is no room below.
	const CRect c = control->getViewSize ();
	const CCoord h = 16.;
	CCoord y = c.bottom + 2.;
	if (y + h > kEditorHeight)
		y = c.top - h - 2.;
	y = std::clamp<CCoord> (y, 0., kEditorHeight - h);
	const CRect r (0., y, static_cast<CCoord> (kEditorWidth), y + h);
	mReadout->setViewSize (r);
	mReadout->setMouseableArea (r);
	mReadout->setAnchorX (c.getCenter ().x);
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
void ForTranEditor::setLiveWaveform (const float*, int) {}

//------------------------------------------------------------------------
void ForTranEditor::setActiveVoices (int count)
{
	if (mVoiceMeter)
		mVoiceMeter->setVoices (std::clamp (count, 0, 16));
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
		if (!mEditing)
			hideReadout ();
	}
}

//------------------------------------------------------------------------
void ForTranEditor::controlBeginEdit (CControl* control)
{
	mEditing = control;
	const auto tag = static_cast<ParamID> (control->getTag ());
	if (mController && (tag < kNumParams || tag == kBypass))
		mController->beginEdit (tag);
	showReadoutFor (control);
}

//------------------------------------------------------------------------
void ForTranEditor::controlEndEdit (CControl* control)
{
	const auto tag = static_cast<ParamID> (control->getTag ());
	if (mController && (tag < kNumParams || tag == kBypass))
		mController->endEdit (tag);
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
	if (tag == kLowPassState || tag == kHighPassState || tag == kCombState || tag == kGrassState)
		return true;
	return tag >= kHarm1 && tag <= kHarm10;
}

//------------------------------------------------------------------------
// The bar graph and the filter screen, both from the parameters alone.
//------------------------------------------------------------------------
void ForTranEditor::refreshDisplays ()
{
	if (!mController)
		return;

	if (mBars)
	{
		std::array<float, 10> levels {};
		for (int k = 0; k < 10; ++k)
			levels[static_cast<size_t> (k)] =
			    static_cast<float> (mController->getParamNormalized (static_cast<ParamID> (kHarm1 + k)));
		mBars->setLevels (levels, mSelected);
	}

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

	if (mFilterOled)
	{
		float curve[FilterInfo::kNumBins];
		mFilter.combinedCurve (states, curve);
		std::vector<float> freq (FilterInfo::kNumBins), mag (FilterInfo::kNumBins);
		float peak = 0.f;
		for (int b = 0; b < FilterInfo::kNumBins; ++b)
		{
			freq[static_cast<size_t> (b)] = mFilter.binFrequency (b);
			mag[static_cast<size_t> (b)] = std::abs (curve[b]);
			if (freq[static_cast<size_t> (b)] >= 20.f && freq[static_cast<size_t> (b)] <= 20000.f)
				peak = std::max (peak, mag[static_cast<size_t> (b)]);
		}
		// Scaled so the loudest point is full height: the screen shows the
		// shape. The figure in the corner says where the cutoff is.
		if (peak > 1e-9f)
			for (auto& m : mag)
				m /= peak;

		// The ten partials of an A3, read off the nearest bin.
		std::array<float, 10> partial {};
		for (int k = 0; k < 10; ++k)
		{
			const float hz = 220.f * static_cast<float> (k + 1);
			int best = 0;
			for (int b = 1; b < FilterInfo::kNumBins; ++b)
				if (std::abs (freq[static_cast<size_t> (b)] - hz) < std::abs (freq[static_cast<size_t> (best)] - hz))
					best = b;
			partial[static_cast<size_t> (k)] = mag[static_cast<size_t> (best)];
		}
		mFilterOled->setResponse (std::move (freq), std::move (mag), partial, mSelected,
		                          valueTextFor (kCutoff));
	}
}

//------------------------------------------------------------------------
bool ForTranEditor::affectsEnvelopes (ParamID tag)
{
	if (tag >= kEnvAttack && tag <= kEnvLevel)             return true;
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
	auto f = [this] (ParamID id) { return static_cast<float> (internalOf (id)); };

	// Read in the same groupings Voice::noteOn uses, so the two cannot drift.
	if (which == 0)
	{
		spec.attack = f (kEnvAttack); spec.decay = f (kEnvDecay);
		spec.sustain = f (kEnvSustain); spec.release = f (kEnvRelease);
		spec.level = f (kEnvLevel);
		spec.enabled = true;
		spec.invert = false;
	}
	else if (which == 1)
	{
		spec.attack = f (kFilterAttack); spec.decay = f (kFilterDecay);
		spec.sustain = f (kFilterSustain); spec.release = f (kFilterRelease);
		spec.level = f (kFilterLevel);
		spec.enabled = true;
		spec.invert = false;
	}
	else
	{
		const int n = std::clamp (mSelected, 0, 9);
		const ParamID base = static_cast<ParamID> (kFormantAttack1 + n * 5);
		spec.attack  = f (base);
		spec.decay   = f (static_cast<ParamID> (base + 1));
		spec.sustain = f (static_cast<ParamID> (base + 2));
		spec.release = f (static_cast<ParamID> (base + 3));
		spec.level   = f (static_cast<ParamID> (base + 4));
		spec.enabled = f (static_cast<ParamID> (kFormantEnv1 + n)) >= 0.5f;
		spec.invert  = f (static_cast<ParamID> (kFormantInv1 + n)) >= 0.5f;
	}
	return spec;
}

//------------------------------------------------------------------------
// The two envelope screens. traceEnvelope drives the real Adsr, so they
// show what the voices will do, on a fixed 0..1 scale so Level shows as
// height.
//------------------------------------------------------------------------
void ForTranEditor::refreshEnvelopes ()
{
	constexpr int kPoints = 220;
	PvEnvOled* screens[2] = { mAmpOled, mPartialOled };
	const int which[2] = { 0, 2 };
	for (int i = 0; i < 2; ++i)
	{
		if (!screens[i])
			continue;
		const EnvelopeSpec spec = envelopeSpec (which[i]);
		std::vector<float> shape (kPoints);
		double keyUp = -1.;
		const double seconds = traceEnvelope (spec, shape.data (), kPoints, &keyUp);
		char time[24] = {};
		if (!spec.enabled)
			std::snprintf (time, sizeof (time), "off");
		else
			formatEnvelopeTime (seconds, time, sizeof (time));
		const std::string caption = (i == 1) ? "FORMANT " + std::to_string (mSelected + 1) : "";
		screens[i]->setEnvelope (std::move (shape), keyUp, caption, upper (time));
	}
}

//------------------------------------------------------------------------
void ForTranEditor::refreshLfos ()
{
	if (!mRings || !mController)
		return;
	std::array<float, 10> depth {};
	std::array<std::string, 10> rate;
	for (int k = 0; k < 10; ++k)
	{
		depth[static_cast<size_t> (k)] =
		    static_cast<float> (mController->getParamNormalized (static_cast<ParamID> (kHarmLfoDepth1 + k)));
		const double hz = harmonicLfoHz (internalOf (static_cast<ParamID> (kHarmLfoRate1 + k)));
		char buf[12];
		std::snprintf (buf, sizeof (buf), hz < 10. ? "%.1f" : "%.0f", hz);
		rate[static_cast<size_t> (k)] = buf;
	}
	mRings->setLfos (depth, rate, mSelected);
}

//------------------------------------------------------------------------
void PLUGIN_API ForTranEditor::close ()
{
	mTimer = nullptr;               // stopped before the frame goes away
	mControls.clear ();
	mValues.clear ();
	for (auto& c : mPartialControls) c = nullptr;
	for (auto& b : mBadges) b = nullptr;
	mEngineText = nullptr;
	mPartialNumber = nullptr;
	mPrevButton = mNextButton = mCopyButton = nullptr;
	mBars = nullptr;
	mFilterOled = nullptr;
	mAmpOled = mPartialOled = nullptr;
	mRings = nullptr;
	mVoiceMeter = nullptr;
	mReadout = nullptr;
	mHovered = nullptr;
	mEditing = nullptr;

	if (frame)
	{
		frame->forget ();
		frame = nullptr;
	}
	mPanelBitmap = nullptr;
	mWearBitmap = nullptr;
}

//------------------------------------------------------------------------
void ForTranEditor::valueChanged (CControl* control)
{
	if (!control || !mController || mUpdating)
		return;

	const int32_t tag = control->getTag ();
	if (tag >= kBadgeTag0 && tag < kBadgeTag0 + 10)
	{
		selectPartial (tag - kBadgeTag0);
		return;
	}

	const ParamID    id    = static_cast<ParamID> (tag);
	const ParamValue value = control->getValueNormalized ();
	mController->setParamNormalized (id, value);
	mController->performEdit (id, value);

	// The other controls bound to the same parameter - a partial's
	// envelope switch is both a lamp and, when selected, the ENV selector.
	mUpdating = true;
	auto it = mControls.find (id);
	if (it != mControls.end ())
		for (auto* c : it->second)
			if (c != control) { c->setValueNormalized (static_cast<float> (value)); c->invalid (); }
	for (auto* c : mPartialControls)
		if (c && c != control && static_cast<ParamID> (c->getTag ()) == id)
		{ c->setValueNormalized (static_cast<float> (value)); c->invalid (); }
	mUpdating = false;

	if (affectsDisplays (id))
		refreshDisplays ();
	if (affectsEnvelopes (id))
		refreshEnvelopes ();
	if (id >= kHarmLfoRate1 && id <= kHarmLfoDepth10)
		refreshLfos ();
	refreshValues ();

	if (mReadout && mReadout->isVisible () && (control == mEditing || control == mHovered))
		mReadout->setText (upper (readoutTextFor (id)));
}

//------------------------------------------------------------------------
void ForTranEditor::updateControl (ParamID tag, ParamValue normalized)
{
	mUpdating = true;
	auto it = mControls.find (tag);
	if (it != mControls.end ())
		for (auto* c : it->second)
		{
			c->setValueNormalized (static_cast<float> (normalized));
			c->invalid ();
		}
	// The partial box is re-pointed, not in mControls.
	for (auto* c : mPartialControls)
		if (c && static_cast<ParamID> (c->getTag ()) == tag)
		{
			c->setValueNormalized (static_cast<float> (normalized));
			c->invalid ();
		}
	mUpdating = false;

	if (affectsDisplays (tag))
		refreshDisplays ();
	if (affectsEnvelopes (tag))
		refreshEnvelopes ();
	if (tag >= kHarmLfoRate1 && tag <= kHarmLfoDepth10)
		refreshLfos ();
	refreshValues ();
}

//------------------------------------------------------------------------
} // namespace ForTran
