//------------------------------------------------------------------------
// ForTran - audio processor
//
// Replaces CForTran (ForTran.cpp), minus the MFX/COM layer.
//
// ForTran is an INSTRUMENT: an event input and one stereo audio output, no
// audio input. The DXi was handed an input buffer and passed it through to
// its output pins; a VST3 instrument has no such buffer and nothing in the
// synthesis depended on it. See PORTING-NOTES section 1.
//
// At this stage the DSP is not ported: process() renders silence. What is
// here is the bus layout, the parameter plumbing and the state, which is
// what the SDK validator exercises.
//------------------------------------------------------------------------

#pragma once

#include "ForTranParams.h"
#include "Synth.h"

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "pluginterfaces/vst/ivstevents.h"

#include <vector>

namespace ForTran {

//------------------------------------------------------------------------
class ForTranProcessor : public Steinberg::Vst::AudioEffect
{
public:
	ForTranProcessor ();

	static Steinberg::FUnknown* createInstance (void*)
	{
		return (Steinberg::Vst::IAudioProcessor*)new ForTranProcessor;
	}

	Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API terminate () SMTG_OVERRIDE;

	Steinberg::tresult PLUGIN_API setBusArrangements (Steinberg::Vst::SpeakerArrangement* inputs,
	                                                  Steinberg::int32 numIns,
	                                                  Steinberg::Vst::SpeakerArrangement* outputs,
	                                                  Steinberg::int32 numOuts) SMTG_OVERRIDE;

	Steinberg::tresult PLUGIN_API canProcessSampleSize (Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;

	Steinberg::tresult PLUGIN_API setupProcessing (Steinberg::Vst::ProcessSetup& setup) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) SMTG_OVERRIDE;

	Steinberg::tresult PLUGIN_API process (Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;

	/** Answers the controller's request for a scope frame. */
	Steinberg::tresult PLUGIN_API notify (Steinberg::Vst::IMessage* message) SMTG_OVERRIDE;

	Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) SMTG_OVERRIDE;

private:
	void applyParameterChanges (Steinberg::Vst::IParameterChanges* changes);
	void sendSampleRateToController ();
	/** Note-on / note-off at one sample offset. */
	void handleEvent (const Steinberg::Vst::Event& event);
	/** Render `numSamples` into the interleaved scratch, then add to the bus. */
	void renderSegment (Steinberg::Vst::ProcessData& data, Steinberg::int32 offset,
	                    Steinberg::int32 numSamples);

	/** Normalised value of every automated parameter, indexed by ParamID. */
	double mParams[kNumParams] = {};
	bool   mBypass = false;
	// Live MIDI controller state, delivered as parameter changes through
	// IMidiMapping. Not part of mParams, and not saved.
	double mMidiVolume = 1.0;
	double mMidiPan    = 0.5;
	double mMidiBend   = 0.5;
	double mSampleRate = 44100.0;

	Synth  mSynth;
	/** Interleaved stereo scratch. The DSP writes interleaved (as the DXi
	    did); VST3 hands out planar buffers, so the two are bridged here. */
	std::vector<float> mScratch;

	/** The output trim, smoothed. A block-rate step on a moving control is
	    a click, and this one is on the mixed output where a click is at its
	    most audible - so the gain walks towards its target one sample at a
	    time, and keeps walking across the segment boundaries that events
	    split a block into. -1 means "not started"; the first block snaps. */
	double mOutputGain   = -1.0;
	double mOutputTarget = 1.0;
	double mOutputCoeff  = 1.0;
};

//------------------------------------------------------------------------
} // namespace ForTran
