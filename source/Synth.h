//------------------------------------------------------------------------
// ForTran - the voice pool
//
// This has no DXi counterpart. A DXi synth never owned its voices: the host
// kept note state in its own event queue (`deque<DXiEvent>`, `de.pvData`)
// and called synthNote once per note per block, so there was no voice limit
// to copy and no allocation policy to port. A VST3 instrument owns both.
//
// No SDK types - see tests/SynthTests.cpp.
//------------------------------------------------------------------------

#pragma once

#include "FilterInfo.h"
#include "Voice.h"

#include <array>
#include <atomic>

namespace ForTran {

//------------------------------------------------------------------------
class Synth
{
public:
	/** The DXi had no polyphony limit; 16 is this port's choice. Each voice
	    carries twelve ADSRs and its own ladder, so this is not free. */
	static constexpr int kMaxVoices = 16;

	void setSampleRate (double sampleRate);
	double sampleRate () const { return mSampleRate; }

	/** Latched into the voice at note-on, as the DXi did. */
	void setVoiceParams (const VoiceParams& params) { mParams = params; }
	VoiceParams& voiceParams () { return mParams; }

	/** The spectral filter's own controls, which are NOT per-note: they
	    rebuild the shared response tables. Cheap to call every block - the
	    rebuild only happens when a value has actually moved. */
	void setFilterControls (float cutoff01, float gain01, float poles01);

	/** Live MIDI controller state: CC7, CC10 and the wheel. Pushed to every
	    sounding voice each block, as the DXi re-read them each block. */
	void setChannelVolume (int volume);
	void setPan (int pan);
	/** `wheel` is 0..1 with 0.5 centred. The DXi's law is 2^((wheel-0.5)*2),
	    i.e. the full throw is a whole OCTAVE either way - much wider than
	    the usual two semitones, and deliberate. */
	void setPitchWheel (double wheel);

	/** The output ladder, in or out. Pushed to every sounding voice each
	    block so it can be switched on a note that is already playing. */
	void setLadderEnabled (bool enabled);

	void noteOn (int noteNumber, int velocity);
	void noteOff (int noteNumber);
	void allNotesOff ();
	void reset ();

	/** Adds into interleaved stereo; does not clear it. Rebuilds the filter
	    tables first if a control has moved. */
	void render (float* stereo, int numSamples);

	int activeVoiceCount () const;

	// -- waveform capture, for the editor's scope --------------------------
	//
	// The DXi captured one cycle of the sounding note in synthNote and its
	// property page pulled it back through MyProps on a 50 ms timer, so the
	// display was alive at 20 fps. VST3 has no such back-channel, so the
	// samples are captured here and handed over on request; see
	// PORTING-NOTES section 3.
	//
	// Written on the audio thread, read on the UI thread, so the handover is
	// a seqlock: no locks, no allocation, and a reader that catches a torn
	// frame simply retries or gives up - a dropped frame at 20 fps costs
	// nothing.
	static constexpr int kCaptureMax = 1024;

	/** Copy the most recent captured frame. Returns the number of samples,
	    or 0 if none is ready or the frame was being overwritten. Safe to
	    call from any thread that is not the audio thread. */
	int readWaveform (float* out, int maxSamples) const;

private:
	Voice* allocateVoice (int noteNumber);

	std::array<Voice, kMaxVoices> mVoices;
	FilterInfo  mFilter;
	VoiceParams mParams;
	double      mSampleRate = 44100.0;
	bool        mLadderEnabled = true;
	int         mChannelVolume = 127;
	int         mPan = 64;
	double      mBendFactor = 1.0;

	// capture state, audio thread only
	int   mCaptureLength = 256;
	int   mCaptureIndex  = 0;
	bool  mCapturing     = false;
	float mCapturePrev   = 0.f;
	float mCaptureWork[kCaptureMax] = {};

	// the handover
	mutable std::atomic<unsigned> mCaptureSeq {0};
	float mCapturePublished[kCaptureMax] = {};
	int   mCapturePublishedLength = 0;

	void captureBlock (const float* stereo, int numSamples);

	// Frequencies are computed rather than read from the DXi's m_adFreq[128]
	// table, which was built with exactly this formula in initTables().
	double noteFrequency (int noteNumber) const;
};

//------------------------------------------------------------------------
} // namespace ForTran
