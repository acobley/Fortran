//------------------------------------------------------------------------
// ForTran - editor
//
// Recreates IDD_PROPPAGE from ForTran.rc. The dialog is 556 x 232 dialog
// units in MS Sans Serif 8pt, so positions convert with
//
//     x_px = x_dlu * 1.5        y_px = y_dlu * 1.625
//
// The canvas is 1064 x 408. The original artwork is 840 x 388;
// tools/make-background.py grows it at the bottom so the painted word
// HARMONICS clears the buttons that used to cover it, and on the right to
// carry three envelope displays the DXi had no counterpart for. The Windows
// dialog was 834 x 377 and clipped its own backdrop; this shows all of it.
//
// Everything from the .rc keeps its original position. The new panels live
// beyond x 840 and are laid out in pixels, since they have no dialog units
// to convert from.
//
// The declared WIDTH AND HEIGHT of most controls are fiction - the knobs
// and the buttons resize themselves at runtime with SWP_NOMOVE, so only x
// and y survive. See PORTING-NOTES section 3.
//------------------------------------------------------------------------

#pragma once

#include "Adsr.h"
#include "FilterInfo.h"
#include "ForTranControls.h"
#include "ForTranParams.h"

#include "public.sdk/source/vst/vstguieditor.h"

#include <map>
#include <string>

namespace ForTran {

class ForTranController;

//------------------------------------------------------------------------
class ForTranEditor : public Steinberg::Vst::VSTGUIEditor,
                     public VSTGUI::IControlListener,
                     public VSTGUI::IKeyboardHook,
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

	// IKeyboardHook - Escape closes the harmonic LFO panel. A hook rather
	// than onKeyboardEvent on the panel itself, because that would need the
	// panel to hold keyboard focus and hosts do not reliably give a
	// plug-in editor's subviews focus.
	void onKeyboardEvent (VSTGUI::KeyboardEvent& event, VSTGUI::CFrame* frame) SMTG_OVERRIDE;

	void updateControl (Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue normalized);

	/** A frame of the real output, from the processor. Overrides the
	    parameter-derived wave until a control is next touched. */
	void setLiveWaveform (const float* samples, int count);

	/** 840 of artwork plus a 224 px strip on the right for the envelope
	    displays. Keep this in step with WIDEN in tools/make-background.py. */
	static const int kEditorWidth  = 1064;

	/** The three envelope panels: amplitude, filter, and whichever harmonic
	    the formant selector is on. */
	static const int kEnvPanelX      = 852;
	static const int kEnvPanelWidth  = 200;
	static const int kEnvPanelHeight = 102;
	/** Tightened from 24/24 to make room below the panels for the output
	    trim and the LFO button; the three panels now end at y 340. */
	static const int kEnvPanelTop    = 6;
	static const int kEnvPanelGap    = 14;

	/** The band below the envelope panels: four controls in a row with
	    their names underneath, then the button that summons the LFO panel.
	    Labels go under rather than beside, because four of them side by
	    side will not fit across 200 px any other way. */
	static const int kStripRowTop    = 346;   // knobs
	static const int kStripLabelTop  = 377;   // their names
	static const int kLfoButtonTop   = 390;

	/** Centres of the four, evenly across the 200 px strip: 852 + 200(i+½)/4.
	    Portamento made this a fourth column; at 50 px apart the labels have
	    to be narrower than they were, which is why the glide one reads
	    "Glide" rather than its full name. The readout says the full name. */
	static const int kStripCol1      = 877;   // output
	static const int kStripCol2      = 927;   // pan
	static const int kStripCol3      = 977;   // portamento
	static const int kStripCol4      = 1027;  // ladder

	/** Half-width of a label box under the strip. 24, not 33, since there
	    are four of them now. */
	static const int kStripLabelHalf = 24;

	/** The harmonic LFO panel. Ten columns on the same 30 px pitch the
	    harmonic sliders use, two rows of knobs, centred over the window. */
	static const int kLfoPanelWidth  = 372;
	static const int kLfoPanelHeight = 146;
	static const int kLfoColumnPitch = 30;
	static const int kLfoColumn0     = 56;   // panel-local x of harmonic 1
	/** 388 in the original artwork; 408 here. The painted word HARMONICS sat
	    under the harmonic invert buttons, so tools/make-background.py moves
	    it down and grows the panel to suit. Keep this in step with the PNG. */
	static const int kEditorHeight = 408;

private:
	VSTGUI::CBitmap* bitmap (const char* name);

	void addKnob (Steinberg::Vst::ParamID tag, double xDlu, double yDlu);
	/** A knob placed in pixels inside `parent`, for the LFO panel, whose
	    layout has no dialog units to convert from. */
	void addKnobAt (Steinberg::Vst::ParamID tag, const VSTGUI::CRect& r,
	                VSTGUI::CViewContainer* parent);
	/** Returns the slider, so the caller can configure it - the ten harmonic
	    sliders take alt-click mute, the cutoff/gain/pole ones do not. */
	FtVSlider* addVSlider (Steinberg::Vst::ParamID tag, double xDlu, double yDlu,
	                       double wDlu, double hDlu);
	void addToggle (Steinberg::Vst::ParamID tag, double xDlu, double yDlu, bool radio = false);
	void registerControl (Steinberg::Vst::ParamID tag, VSTGUI::CControl* control);

	/** The ten formant buttons choose which formant the five formant knobs
	    edit - that is what the DXi's _PARAM_FORMANT_ held. It is editor
	    state, not a parameter. */
	void selectFormant (int index);
	void retargetFormantControls ();

	/** Recompute the two display panels. Both are derived from the
	    parameters alone, so the editor can draw them without any channel
	    back from the processor - see PORTING-NOTES section 3. */
	void refreshDisplays ();
	static bool affectsDisplays (Steinberg::Vst::ParamID tag);

	/** Redraw the three envelope panels. Separate from refreshDisplays
	    because the envelopes and the filter/waveform panels are driven by
	    disjoint sets of parameters, and tracing three envelopes on every
	    harmonic slider movement would be wasted work. */
	void refreshEnvelopes ();
	static bool affectsEnvelopes (Steinberg::Vst::ParamID tag);

	/** The settings for one envelope, read from the parameters exactly as
	    Voice::noteOn reads them - so the drawing and the sound are built
	    from the same numbers. `which` is 0 amplitude, 1 filter, 2 the
	    selected harmonic. */
	EnvelopeSpec envelopeSpec (int which) const;

	/** The shared, transient value readout. */
	void showReadoutFor (VSTGUI::CControl* control);
	void hideReadout ();
	std::string readoutTextFor (Steinberg::Vst::ParamID tag) const;

	ForTranController* mController = nullptr;

	std::map<std::string, VSTGUI::SharedPointer<VSTGUI::CBitmap>> mBitmaps;
	std::map<Steinberg::Vst::ParamID, VSTGUI::CControl*> mControls;

	/** The five formant knobs plus the ENV and INV toggles, in the order
	    attack, decay, sustain, release, level, env, invert. Their tags are
	    re-pointed when the selected formant changes. */
	VSTGUI::CControl* mFormantControls[7] = {};
	VSTGUI::CControl* mFormantButtons[10] = {};
	FtValueDisplay* mReadout = nullptr;
	VSTGUI::CControl* mHovered = nullptr;
	VSTGUI::CControl* mEditing = nullptr;

	/** Build the harmonic LFO panel and the sheet behind it, and show or
	    hide the pair. */
	void buildLfoPanel ();
	void showLfoPanel (bool show);

	VSTGUI::CViewContainer* mLfoPanel = nullptr;
	VSTGUI::CView* mLfoScrim = nullptr;

	FtCurveView* mResponseView = nullptr;
	FtCurveView* mWaveView = nullptr;
	/** Amplitude, filter, selected harmonic - in that order, top to bottom. */
	FtCurveView* mEnvViews[3] = {};
	FilterInfo   mFilter;
	/** Drives the scope: asks the processor for a frame 20 times a second,
	    which is the rate the DXi's property page ran its own timer at. */
	VSTGUI::SharedPointer<VSTGUI::CVSTGUITimer> mScopeTimer;

	int  mSelectedFormant = 0;
	bool mUpdating = false;
};

//------------------------------------------------------------------------
} // namespace ForTran
