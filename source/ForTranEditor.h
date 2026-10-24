//------------------------------------------------------------------------
// ForTran - editor: the Polivoks panel
//
// 1200 x 640. A matte black panel with white silkscreen, black knurled
// knobs, chicken-head rotary switches, red jewel lamps and small colour
// OLEDs behind glass, lettered throughout in Andy's 70s sci-fi face, with a
// light studio-worn finish. The design is Designs/
// ForTran-Polivoks-2b-SpectrumBank-StudioWorn.png; its source is
// tools/polivoks/d2p.js.
//
// Three layers, bottom to top:
//
//   resource/pv_panel.png  everything that never changes - boxes, titles,
//                          legends, printed knob scales, OLED bezels. 2x.
//   the controls           drawn with paths (PvControls), at the very
//                          coordinates d2p.js uses. Change one, change both.
//   resource/pv_wear.png   dust, hairlines, polish and fingerprints, as a
//                          transparent layer the mouse passes through. 2x.
//
// The ten partials share ONE editing box, as the DXi's formant knobs did:
// the numbered rings choose which partial the box (and the selected bar,
// dot and ring on the OLEDs) is showing. That choice is editor state, not a
// parameter. Everything else is on the panel at once - the partial bank
// has every partial's level, phase, invert and envelope switch.
//------------------------------------------------------------------------

#pragma once

#include "Adsr.h"
#include "FilterInfo.h"
#include "ForTranControls.h"
#include "ForTranParams.h"
#include "PvControls.h"

#include "public.sdk/source/vst/vstguieditor.h"

#include <map>
#include <string>
#include <vector>

namespace ForTran {

class ForTranController;

//------------------------------------------------------------------------
class ForTranEditor : public Steinberg::Vst::VSTGUIEditor,
                     public VSTGUI::IControlListener,
                     public IHoverListener
{
public:
	explicit ForTranEditor (ForTranController* controller);

	bool PLUGIN_API open (void* parent, const VSTGUI::PlatformType& platformType) SMTG_OVERRIDE;
	void PLUGIN_API close () SMTG_OVERRIDE;

	void valueChanged (VSTGUI::CControl* control) SMTG_OVERRIDE;
	void controlBeginEdit (VSTGUI::CControl* control) SMTG_OVERRIDE;
	void controlEndEdit (VSTGUI::CControl* control) SMTG_OVERRIDE;

	// IHoverListener
	void controlHovered (VSTGUI::CControl* control, bool entered) SMTG_OVERRIDE;

	void updateControl (Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue normalized);

	/** A frame of the real output, from the processor. The Polivoks panel
	    has no scope, so this is ignored; the message still carries the
	    voice count below. */
	void setLiveWaveform (const float* samples, int count);

	/** How many voices are sounding, for the header's meter. */
	void setActiveVoices (int count);

	static const int kEditorWidth  = 1200;
	static const int kEditorHeight = 640;

private:
	VSTGUI::CBitmap* bitmap2x (const char* name);

	PvKnob* addKnob (Steinberg::Vst::ParamID tag, double cx, double cy, double outer);
	/** The value printed under (or beside) a knob. */
	void addValue (VSTGUI::CControl* control, double x, double y, double size,
	               SciFi::Align align = SciFi::Align::Centre, VSTGUI::CColor colour = Pv::kInk);
	PvSelector* addSelector (Steinberg::Vst::ParamID tag, double cx, double cy, double r,
	                         std::vector<std::string> names, double legendSize, double legendRadius);
	PvLamp* addLamp (Steinberg::Vst::ParamID tag, double cx, double cy);
	void registerControl (Steinberg::Vst::ParamID tag, VSTGUI::CControl* control);
	void setDefault (VSTGUI::CControl* control, Steinberg::Vst::ParamID tag);

	void selectPartial (int index);
	void retargetPartialControls ();
	void copyPartialToAll ();

	void refreshDisplays ();
	static bool affectsDisplays (Steinberg::Vst::ParamID tag);
	void refreshEnvelopes ();
	static bool affectsEnvelopes (Steinberg::Vst::ParamID tag);
	EnvelopeSpec envelopeSpec (int which) const;   // 0 amp, 1 filter, 2 selected partial
	void refreshLfos ();
	void refreshValues ();
	void refreshHeader ();

	void showReadoutFor (VSTGUI::CControl* control);
	void hideReadout ();
	std::string readoutTextFor (Steinberg::Vst::ParamID tag) const;
	/** The short form printed on the panel: the readout without its name,
	    with cutoff as 2.40K rather than 2400 Hz. */
	std::string valueTextFor (Steinberg::Vst::ParamID tag) const;

	double internalOf (Steinberg::Vst::ParamID id) const;

	ForTranController* mController = nullptr;

	VSTGUI::SharedPointer<VSTGUI::CBitmap> mPanelBitmap, mWearBitmap;

	/** Every control bound to a fixed parameter. A vector, because a
	    partial's envelope switch appears twice - its lamp in the bank and,
	    when it is the selected one, the ENV selector in the box; the box's
	    copies are in mPartialControls and re-pointed, not listed here. */
	std::map<Steinberg::Vst::ParamID, std::vector<VSTGUI::CControl*>> mControls;

	/** The partial box: attack, decay, sustain, release, level, the ENV
	    selector, the INV lamp, LFO rate and LFO depth. */
	enum { kPA, kPD, kPS, kPR, kPLevel, kPEnv, kPInv, kPRate, kPDepth, kNumPartialControls };
	VSTGUI::CControl* mPartialControls[kNumPartialControls] = {};
	PvBadge* mBadges[10] = {};

	struct ValueSlot { VSTGUI::CControl* control; PvText* text; };
	std::vector<ValueSlot> mValues;

	PvText*       mEngineText = nullptr;
	PvText*       mPartialNumber = nullptr;
	PvButton*     mPrevButton = nullptr;
	PvButton*     mNextButton = nullptr;
	PvButton*     mCopyButton = nullptr;
	PvBarsOled*   mBars = nullptr;
	PvFilterOled* mFilterOled = nullptr;
	PvEnvOled*    mAmpOled = nullptr;
	PvEnvOled*    mPartialOled = nullptr;
	PvRingsOled*  mRings = nullptr;
	PvVoiceMeter* mVoiceMeter = nullptr;
	PvReadout*    mReadout = nullptr;
	VSTGUI::CControl* mHovered = nullptr;
	VSTGUI::CControl* mEditing = nullptr;

	FilterInfo mFilter;
	/** Asks the processor for its voice count 20 times a second. */
	VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> mTimer;

	int  mSelected = 0;
	bool mUpdating = false;
};

//------------------------------------------------------------------------
} // namespace ForTran
