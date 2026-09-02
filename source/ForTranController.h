//------------------------------------------------------------------------
// ForTran - edit controller
//
// Replaces CMediaParams plus CForTranPropPage's parameter plumbing.
// No editor yet; createView is added in phase 5.
//------------------------------------------------------------------------

#pragma once

#include "ForTranParams.h"

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"

#include <vector>

namespace ForTran {

class ForTranEditor;

//------------------------------------------------------------------------
class ForTranController : public Steinberg::Vst::EditControllerEx1,
                          public Steinberg::Vst::IMidiMapping
{
public:
	OBJ_METHODS (ForTranController, Steinberg::Vst::EditControllerEx1)
	DEFINE_INTERFACES
		DEF_INTERFACE (Steinberg::Vst::IMidiMapping)
	END_DEFINE_INTERFACES (Steinberg::Vst::EditControllerEx1)
	DELEGATE_REFCOUNT (Steinberg::Vst::EditControllerEx1)

	ForTranController () = default;
	~ForTranController () SMTG_OVERRIDE = default;

	static Steinberg::FUnknown* createInstance (void*)
	{
		return (Steinberg::Vst::IEditController*)new ForTranController;
	}

	Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setComponentState (Steinberg::IBStream* state) SMTG_OVERRIDE;

	Steinberg::tresult PLUGIN_API notify (Steinberg::Vst::IMessage* message) SMTG_OVERRIDE;

	Steinberg::IPlugView* PLUGIN_API createView (Steinberg::FIDString name) SMTG_OVERRIDE;

	Steinberg::tresult PLUGIN_API setParamNormalized (Steinberg::Vst::ParamID tag,
	                                                  Steinberg::Vst::ParamValue value) SMTG_OVERRIDE;

	/** Routes CC7, CC10 and the pitch wheel to kMidiVolume / kMidiPan /
	    kMidiPitchBend. Without this the host has nowhere to deliver them
	    and the plug-in simply never sees a controller. */
	Steinberg::tresult PLUGIN_API getMidiControllerAssignment (
	    Steinberg::int32 busIndex, Steinberg::int16 channel,
	    Steinberg::Vst::CtrlNumber midiControllerNumber,
	    Steinberg::Vst::ParamID& id) SMTG_OVERRIDE;

	void editorAttached (Steinberg::Vst::EditorView* editor) SMTG_OVERRIDE;
	void editorRemoved (Steinberg::Vst::EditorView* editor) SMTG_OVERRIDE;
	void editorDestroyed (Steinberg::Vst::EditorView* editor) SMTG_OVERRIDE;

	/** The sample rate the processor is running at. The editor will need it
	    to draw the filter-response and waveform panels. */
	double getHostSampleRate () const { return mHostSampleRate; }

	/** Ask the processor for a scope frame. Called from the editor's timer. */
	void requestWaveFrame ();

private:
	std::vector<ForTranEditor*> mEditors;
	double mHostSampleRate = 44100.0;
};

//------------------------------------------------------------------------
} // namespace ForTran
