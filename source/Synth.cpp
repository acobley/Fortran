//------------------------------------------------------------------------
// ForTran - voice pool implementation
//------------------------------------------------------------------------

#include "Synth.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace ForTran {

//------------------------------------------------------------------------
void Synth::setSampleRate (double sampleRate)
{
	mSampleRate = (sampleRate > 1000.0) ? sampleRate : 44100.0;
	reset ();
}

//------------------------------------------------------------------------
double Synth::noteFrequency (int noteNumber) const
{
	// CForTran::initTables filled m_adFreq[128] as 440 Hz at note 69, twelve
	// equal steps to the octave.
	return 440.0 * std::pow (2.0, (noteNumber - 69) / 12.0);
}

//------------------------------------------------------------------------
void Synth::setFilterControls (float cutoff01, float gain01, float poles01)
{
	// The DXi's ChangeFilter(): gain is scaled by 3.9, poles are 1 + 10x and
	// truncated to an integer. Each setter marks the tables dirty only if
	// the value actually moved.
	mFilter.setCutoff (cutoff01);
	mFilter.setFeedback (gain01 * 3.9f);
	mFilter.setPoles (1.f + poles01 * 10.f);
}

//------------------------------------------------------------------------
void Synth::reset ()
{
	for (Voice& v : mVoices)
		v = Voice {};
}

//------------------------------------------------------------------------
Voice* Synth::allocateVoice (int noteNumber)
{
	// Retrigger the same note if it is already sounding, so a repeated
	// note-on does not stack two voices on one pitch.
	for (Voice& v : mVoices)
		if (v.isActive () && !v.isReleased () && v.noteNumber () == noteNumber)
			return &v;

	for (Voice& v : mVoices)
		if (!v.isActive ())
			return &v;

	// All busy. Prefer to steal a released voice - it is already fading -
	// and take the oldest of them; otherwise the oldest voice of all.
	Voice* best = nullptr;
	for (Voice& v : mVoices)
	{
		if (!v.isReleased ())
			continue;
		if (!best || v.age () > best->age ())
			best = &v;
	}
	if (best)
		return best;

	best = &mVoices[0];
	for (Voice& v : mVoices)
		if (v.age () > best->age ())
			best = &v;
	return best;
}

//------------------------------------------------------------------------
void Synth::setChannelVolume (int volume)
{
	mChannelVolume = std::clamp (volume, 0, 127);
	for (Voice& v : mVoices)
		if (v.isActive ())
			v.setChannelVolume (mChannelVolume);
}

//------------------------------------------------------------------------
void Synth::setPan (int pan)
{
	mPan = std::clamp (pan, 0, 127);
	for (Voice& v : mVoices)
		if (v.isActive ())
			v.setPan (mPan);
}

//------------------------------------------------------------------------
void Synth::setPitchWheel (double wheel)
{
	// DXi: fWheel = (nWheel - centre)/centre, in -1..+1; fPitch = 2^fWheel.
	const double f = std::pow (2.0, std::clamp (wheel, 0.0, 1.0) * 2.0 - 1.0);
	mBendFactor = f;
	for (Voice& v : mVoices)
		if (v.isActive ())
			v.setPitchBend (f);
}

//------------------------------------------------------------------------
void Synth::noteOn (int noteNumber, int velocity)
{
	if (velocity <= 0)
	{
		// A note-on at velocity zero is a note-off; hosts and files both do
		// this and a synth that ignores it hangs notes.
		noteOff (noteNumber);
		return;
	}

	if (Voice* v = allocateVoice (noteNumber))
	{
		v->noteOn (noteNumber, noteFrequency (noteNumber), velocity,
		           mChannelVolume, mPan, mParams, mSampleRate);
		v->setPitchBend (mBendFactor);

		// Two periods of this note, as the DXi sized its capture
		// (WaveSampleLength = 2 * sampleRate / frequency, capped).
		const double period = mSampleRate / noteFrequency (noteNumber);
		mCaptureLength = std::clamp (static_cast<int> (2.0 * period), 64, kCaptureMax);
	}
}

//------------------------------------------------------------------------
void Synth::noteOff (int noteNumber)
{
	for (Voice& v : mVoices)
		if (v.isActive () && !v.isReleased () && v.noteNumber () == noteNumber)
			v.noteOff ();
}

//------------------------------------------------------------------------
void Synth::allNotesOff ()
{
	for (Voice& v : mVoices)
		if (v.isActive ())
			v.noteOff ();
}

//------------------------------------------------------------------------
void Synth::render (float* stereo, int numSamples)
{
	// Once per block, and only when a control has moved. About 2,200
	// transcendentals; see PORTING-NOTES section 6.
	if (mFilter.needsRecalculation ())
		mFilter.calculate ();

	for (Voice& v : mVoices)
		if (v.isActive ())
			v.render (stereo, numSamples, mFilter);

	captureBlock (stereo, numSamples);
}

//------------------------------------------------------------------------
// Waveform capture.
//
// The trigger is the DXi's: a RISING zero crossing, tested as
// (prev != 0) && (prev * cur < 0) && (prev <= 0). Starting from a consistent
// point in the cycle is what stops the picture sliding sideways.
//
// Unlike the DXi this watches the MIXED output rather than one voice.
// synthNote captured per note into a single shared buffer, so with a chord
// each note overwrote the last and the display showed whichever happened to
// render last. The mix is simpler and closer to what you are hearing.
//------------------------------------------------------------------------
void Synth::captureBlock (const float* stereo, int numSamples)
{
	for (int i = 0; i < numSamples; ++i)
	{
		const float cur = 0.5f * (stereo[2 * i] + stereo[2 * i + 1]);

		if (!mCapturing)
		{
			if (mCapturePrev != 0.f && mCapturePrev * cur < 0.f && mCapturePrev <= 0.f)
			{
				mCapturing = true;
				mCaptureIndex = 0;
			}
		}

		if (mCapturing)
		{
			mCaptureWork[mCaptureIndex++] = cur;
			if (mCaptureIndex >= mCaptureLength)
			{
				// Publish. An odd sequence means "being written"; a reader
				// that sees an odd value, or a different one either side of
				// its copy, discards the frame and tries again.
				mCaptureSeq.fetch_add (1, std::memory_order_acq_rel);
				std::memcpy (mCapturePublished, mCaptureWork,
				             sizeof (float) * static_cast<size_t> (mCaptureLength));
				mCapturePublishedLength = mCaptureLength;
				mCaptureSeq.fetch_add (1, std::memory_order_release);

				mCapturing = false;
			}
		}

		mCapturePrev = cur;
	}
}

//------------------------------------------------------------------------
int Synth::readWaveform (float* out, int maxSamples) const
{
	for (int attempt = 0; attempt < 4; ++attempt)
	{
		const unsigned before = mCaptureSeq.load (std::memory_order_acquire);
		if (before & 1u)
			continue;                       // a write is in progress

		const int n = std::min (mCapturePublishedLength, maxSamples);
		if (n <= 0)
			return 0;
		std::memcpy (out, mCapturePublished, sizeof (float) * static_cast<size_t> (n));

		if (mCaptureSeq.load (std::memory_order_acquire) == before)
			return n;                       // untouched while we copied
	}
	return 0;                               // torn every time; skip this frame
}

//------------------------------------------------------------------------
int Synth::activeVoiceCount () const
{
	int n = 0;
	for (const Voice& v : mVoices)
		if (v.isActive ())
			++n;
	return n;
}

//------------------------------------------------------------------------
} // namespace ForTran
