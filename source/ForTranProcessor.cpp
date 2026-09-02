//------------------------------------------------------------------------
// ForTran - audio processor implementation
//------------------------------------------------------------------------

#include "ForTranProcessor.h"
#include "ForTranIDs.h"
#include "VoiceParamsMap.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include <algorithm>
#include <cmath>
#include <cstring>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace ForTran {

//------------------------------------------------------------------------
ForTranProcessor::ForTranProcessor ()
{
	setControllerClass (kForTranControllerUID);

	for (int i = 0; i < kNumParams; ++i)
		mParams[i] = kParams[i].defaultNormalized ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API ForTranProcessor::initialize (FUnknown* context)
{
	tresult result = AudioEffect::initialize (context);
	if (result != kResultOk)
		return result;

	// An instrument: notes in, audio out, no audio input.
	addAudioOutput (STR16 ("Stereo Out"), SpeakerArr::kStereo);
	addEventInput (STR16 ("Event In"), 16);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API ForTranProcessor::terminate ()
{
	return AudioEffect::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API ForTranProcessor::setBusArrangements (SpeakerArrangement* inputs, int32 numIns,
                                                         SpeakerArrangement* outputs, int32 numOuts)
{
	// Stereo out, nothing in. This must agree with the AudioComponents entry
	// in resource/au-info.plist (0 in / 2 out) or auval will reject the AU.
	if (numIns == 0 && numOuts == 1 && outputs[0] == SpeakerArr::kStereo)
		return AudioEffect::setBusArrangements (inputs, numIns, outputs, numOuts);

	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API ForTranProcessor::canProcessSampleSize (int32 symbolicSampleSize)
{
	// The DXi accepted 32-bit float only (IsValidInputFormat). Hosts offer
	// 64-bit buffers, so both are accepted here.
	if (symbolicSampleSize == kSample32 || symbolicSampleSize == kSample64)
		return kResultTrue;
	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API ForTranProcessor::setupProcessing (ProcessSetup& setup)
{
	mSampleRate = setup.sampleRate;

	// Allocation belongs here, not in process(). The DXi reallocated from
	// inside its processing loop; SpaceDub had the same problem.
	mSynth.setSampleRate (mSampleRate);
	mScratch.assign (static_cast<size_t> (setup.maxSamplesPerBlock) * 2, 0.f);

	// About 10 ms to settle, which is inaudible on a fader move and far
	// faster than anyone can turn a knob.
	mOutputCoeff = 1.0 - std::exp (-1.0 / (0.01 * mSampleRate));
	mOutputGain = -1.0;                        // snap to the target next block

	return AudioEffect::setupProcessing (setup);
}

//------------------------------------------------------------------------
tresult PLUGIN_API ForTranProcessor::setActive (TBool state)
{
	if (state)
		sendSampleRateToController ();
	return AudioEffect::setActive (state);
}

//------------------------------------------------------------------------
void ForTranProcessor::sendSampleRateToController ()
{
	// setActive is [UI-thread], so a message is legitimate here. It would not
	// be from process() - see PORTING-GUIDE section 7.
	if (auto* message = allocateMessage ())
	{
		FReleaser releaser (message);
		message->setMessageID (kForTranSampleRateMessage);
		message->getAttributes ()->setFloat (kForTranSampleRateAttribute, mSampleRate);
		sendMessage (message);
	}
}

//------------------------------------------------------------------------
void ForTranProcessor::applyParameterChanges (IParameterChanges* changes)
{
	if (!changes)
		return;

	const int32 count = changes->getParameterCount ();
	for (int32 i = 0; i < count; ++i)
	{
		IParamValueQueue* queue = changes->getParameterData (i);
		if (!queue)
			continue;

		const int32 points = queue->getPointCount ();
		if (points <= 0)
			continue;

		// The value at the end of the block is the target.
		int32      sampleOffset = 0;
		ParamValue value        = 0.0;
		if (queue->getPoint (points - 1, sampleOffset, value) != kResultTrue)
			continue;

		const ParamID id = queue->getParameterId ();
		if (id == kBypass)        { mBypass = (value >= 0.5); continue; }
		if (id == kMidiVolume)    { mMidiVolume = value; continue; }
		if (id == kMidiPan)       { mMidiPan = value; continue; }
		if (id == kMidiPitchBend) { mMidiBend = value; continue; }

		// Range-check before indexing: kBypass is 1000, far past the array.
		if (id < kNumParams)
			mParams[id] = value;
	}
}

//------------------------------------------------------------------------
void ForTranProcessor::handleEvent (const Event& event)
{
	switch (event.type)
	{
		case Event::kNoteOnEvent:
		{
			// VST3 velocity is 0..1; the DXi's gain law wants 0..127.
			const int velocity = std::clamp (
			    static_cast<int> (event.noteOn.velocity * 127.f + 0.5f), 0, 127);
			// Channel volume, pan and the wheel arrive as parameter changes
			// through IMidiMapping and are applied above, per block.
			mSynth.noteOn (event.noteOn.pitch, velocity);
			break;
		}

		case Event::kNoteOffEvent:
			mSynth.noteOff (event.noteOff.pitch);
			break;

		default:
			break;
	}
}

//------------------------------------------------------------------------
void ForTranProcessor::renderSegment (ProcessData& data, int32 offset, int32 numSamples)
{
	if (numSamples <= 0)
		return;

	const size_t needed = static_cast<size_t> (numSamples) * 2;
	if (mScratch.size () < needed)
		return;                       // setupProcessing sized this; never grow here

	std::fill_n (mScratch.begin (), needed, 0.f);

	// "Enabled" is the DXi's own switch and defaults ON, unlike SpaceDub's.
	// Bypass is the separate VST3 one. Events are still handled when either
	// is off, so notes do not hang.
	const bool enabled = (mParams[kEnable] >= 0.5) && !mBypass;
	if (enabled)
		mSynth.render (mScratch.data (), numSamples);

	// The output trim, on the mix. See the note in ForTranParams.h: the
	// DXi's own staging is the top of this control, and its default is 20 dB
	// below that because the original's gain runs above unity on one note.
	{
		if (mOutputGain < 0.0)
			mOutputGain = mOutputTarget;       // first block, no ramp
		for (int32 i = 0; i < numSamples; ++i)
		{
			mOutputGain += (mOutputTarget - mOutputGain) * mOutputCoeff;
			const float g = static_cast<float> (mOutputGain);
			mScratch[static_cast<size_t> (i) * 2]     *= g;
			mScratch[static_cast<size_t> (i) * 2 + 1] *= g;
		}
	}

	AudioBusBuffers& out = data.outputs[0];
	const bool isDouble = (processSetup.symbolicSampleSize == kSample64);

	for (int32 ch = 0; ch < out.numChannels; ++ch)
	{
		const int32 src = (ch < 2) ? ch : 1;
		if (isDouble)
		{
			if (!out.channelBuffers64 || !out.channelBuffers64[ch]) continue;
			Sample64* dst = out.channelBuffers64[ch] + offset;
			for (int32 i = 0; i < numSamples; ++i)
				dst[i] = mScratch[static_cast<size_t> (i) * 2 + src];
		}
		else
		{
			if (!out.channelBuffers32 || !out.channelBuffers32[ch]) continue;
			Sample32* dst = out.channelBuffers32[ch] + offset;
			for (int32 i = 0; i < numSamples; ++i)
				dst[i] = mScratch[static_cast<size_t> (i) * 2 + src];
		}
	}
}

//------------------------------------------------------------------------
tresult PLUGIN_API ForTranProcessor::process (ProcessData& data)
{
	applyParameterChanges (data.inputParameterChanges);

	mOutputTarget = outputGain (mParams[kOutputLevel]);

	// The spectral filter's controls are not per-note: they rebuild the
	// shared response tables. Handing them over every block is cheap - the
	// rebuild only runs when a value has actually moved.
	mSynth.setFilterControls (
	    static_cast<float> (paramDef (kCutoff).toInternal (mParams[kCutoff])),
	    static_cast<float> (paramDef (kFilterGain).toInternal (mParams[kFilterGain])),
	    static_cast<float> (paramDef (kPoles).toInternal (mParams[kPoles])));

	// Everything else is latched into the voice at note-on, as the DXi did.
	buildVoiceParams (mParams, mSynth.voiceParams ());

	// ...except channel volume, pan and the wheel, which the DXi re-read in
	// synthNote's prologue every block, so they follow the controller live.
	mSynth.setChannelVolume (static_cast<int> (mMidiVolume * 127.0 + 0.5));
	// The Pan knob offsets CC10 rather than replacing it, so a host that
	// automates either still works and one that sends neither starts centred.
	mSynth.setPan (combinePan (static_cast<int> (mMidiPan * 127.0 + 0.5),
	                           paramDef (kPan).toInternal (mParams[kPan])));
	mSynth.setPitchWheel (mMidiBend);

	// The ladder switch goes the same way, so it can be thrown on a note
	// that is already sounding - which is the only way to judge a filter
	// that does nothing below B4.
	mSynth.setLadderEnabled (mParams[kLadderEnable] >= 0.5);

	// No output bus, or nothing to do: still consume the events, so a
	// note-off is never dropped and a note cannot hang.
	if (data.numOutputs <= 0 || data.outputs == nullptr || data.numSamples <= 0)
	{
		if (auto* events = data.inputEvents)
		{
			const int32 count = events->getEventCount ();
			for (int32 i = 0; i < count; ++i)
			{
				Event e;
				if (events->getEvent (i, e) == kResultOk)
					handleEvent (e);
			}
		}
		return kResultOk;
	}

	// Sample-accurate: render up to each event, apply it, carry on.
	int32 position = 0;
	if (auto* events = data.inputEvents)
	{
		const int32 count = events->getEventCount ();
		for (int32 i = 0; i < count; ++i)
		{
			Event e;
			if (events->getEvent (i, e) != kResultOk)
				continue;

			const int32 at = std::clamp (e.sampleOffset, position, data.numSamples);
			renderSegment (data, position, at - position);
			position = at;
			handleEvent (e);
		}
	}
	renderSegment (data, position, data.numSamples - position);

	// Tell the host when there is genuinely nothing sounding, so it can skip
	// downstream work.
	data.outputs[0].silenceFlags =
	    (mSynth.activeVoiceCount () == 0)
	        ? ((data.outputs[0].numChannels >= 64) ? ~0ULL
	                                               : ((1ULL << data.outputs[0].numChannels) - 1))
	        : 0;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API ForTranProcessor::notify (IMessage* message)
{
	if (message && FIDStringsEqual (message->getMessageID (), kForTranWaveRequestMessage))
	{
		// Called on the UI thread, so replying with a message is legitimate.
		// The frame itself was captured on the audio thread and handed over
		// through a seqlock; readWaveform returns 0 rather than blocking if
		// it catches a write in progress, and a dropped frame at 20 fps is
		// invisible.
		float frame[Synth::kCaptureMax];
		const int n = mSynth.readWaveform (frame, Synth::kCaptureMax);
		if (n > 0)
		{
			if (auto* reply = allocateMessage ())
			{
				FReleaser releaser (reply);
				reply->setMessageID (kForTranWaveDataMessage);
				reply->getAttributes ()->setBinary (
				    kForTranWaveAttribute, frame,
				    static_cast<uint32> (n * sizeof (float)));
				sendMessage (reply);
			}
		}
		return kResultOk;
	}
	return AudioEffect::notify (message);
}

//------------------------------------------------------------------------
tresult PLUGIN_API ForTranProcessor::getState (IBStream* state)
{
	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);

	streamer.writeInt32 (1);              // stream version
	streamer.writeInt32 (kNumParams);
	for (int i = 0; i < kNumParams; ++i)
		streamer.writeDouble (mParams[i]);
	streamer.writeInt32 (mBypass ? 1 : 0);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API ForTranProcessor::setState (IBStream* state)
{
	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);

	int32 version = 0;
	if (!streamer.readInt32 (version))
		return kResultFalse;

	int32 count = 0;
	if (!streamer.readInt32 (count))
		return kResultFalse;

	for (int32 i = 0; i < count; ++i)
	{
		double v = 0.0;
		if (!streamer.readDouble (v))
			return kResultFalse;
		if (i < kNumParams)
			mParams[i] = v;
	}

	// A project saved before a parameter existed carries a shorter stream.
	// Everything it does not mention goes back to its default rather than
	// keeping whatever the previous patch left in this instance - loading an
	// old project after a new one must not inherit the new one's settings.
	for (int32 i = count; i < kNumParams; ++i)
		mParams[i] = kParams[i].defaultNormalized ();

	int32 bypass = 0;
	if (streamer.readInt32 (bypass))
		mBypass = (bypass != 0);

	return kResultOk;
}

//------------------------------------------------------------------------
} // namespace ForTran
