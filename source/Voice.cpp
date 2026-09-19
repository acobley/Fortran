//------------------------------------------------------------------------
// ForTran - voice implementation
//------------------------------------------------------------------------

#include "Voice.h"

#include <algorithm>
#include <cmath>

namespace ForTran {

namespace {
constexpr double dPI  = 3.14159265358979323846264338327950288419716939937510;
constexpr double d2PI = dPI * 2.0;
/** The DXi's dPIm - the phase offsets are built with a NEGATIVE pi. */
constexpr double dPIm = -dPI;

/** The ladder's cutoff and resonance are hardcoded in the DXi:
        double f  = 0.50 * 1.16;
        double fb = 0.5 * (1.0 - 0.15 * f * f);
        double ffff = 0.35013 * (f*f)*(f*f);
    No parameter reaches them - the Filter Cutoff control drives the
    spectral filter, not this. */
constexpr double kLadderF  = 0.50 * 1.16;
const     double kLadderFb = 0.5 * (1.0 - 0.15 * kLadderF * kLadderF);
const     double kLadderFf = 0.35013 * (kLadderF * kLadderF) * (kLadderF * kLadderF);

/** What the ladder does to a signal well below its corner, which is a
    fixed gain of 3.39 - and nothing else.

    Each section is `y = x + 0.3*x' + (1-f)*y'`, so at DC it multiplies by
    1.3/f; four of them give (1.3/f)^4, the feed-forward scales that (to
    almost exactly 1, which cannot be chance), and the feedback loop divides
    by 1+fb. The times 5 at the end is the original's.

    Derived from the same three constants rather than written down as a
    measured number, so it cannot drift away from the filter it matches. */
const double kLadderDcGain =
    5.0 * (kLadderFf * std::pow (1.3 / kLadderF, 4.0)) /
    (1.0 + kLadderFb * (kLadderFf * std::pow (1.3 / kLadderF, 4.0)));

/** The DXi built its LFO steps by dividing by the rate control:
        dLFOStepVibrato = 2*pi / (rate * sampleRate)
    so the control is really a PERIOD in seconds, and a rate of zero - the
    bottom of a control whose minimum is zero - divides by zero. The same
    applies to the tremolo rate, and to the vibrato depth, which divides by
    (1 - depth) and so blows up at the top of its travel.

    One millisecond is the floor, matching Adsr: far below any usable
    setting, and the only place the original produced an infinity. */
inline double lfoStep (double rateSeconds, double sampleRate)
{
	const double minSeconds = 0.001;
	if (!(rateSeconds > minSeconds))      // also catches NaN
		rateSeconds = minSeconds;
	return d2PI / (rateSeconds * sampleRate);
}
} // anonymous namespace

//------------------------------------------------------------------------
void Voice::noteOn (int noteNumber, double frequency, int velocity,
                    int channelVolume, int pan,
                    const VoiceParams& params, double sampleRate,
                    double glideFromHz)
{
	mActive        = true;
	mReleased      = false;
	mNoteNumber    = noteNumber;
	mCurrentSample = 0;

	mTheta = 0.0;
	mStep  = d2PI * frequency / sampleRate;

	// -- portamento --------------------------------------------------------
	//
	// A glide needs somewhere to come from, a time, and the two pitches to
	// differ; any of those missing and this is the note starting on pitch,
	// which is what the plug-in did before the control existed.
	mStepTarget     = mStep;
	mGlideRatio     = 1.0;
	mGlideRemaining = 0;

	if (params.portamento > 0.f && glideFromHz > 0.0 && frequency > 0.0
	    && glideFromHz != frequency)
	{
		const double samples = static_cast<double> (params.portamento) * sampleRate;
		const long   n       = static_cast<long> (samples + 0.5);
		if (n > 0)
		{
			mStep           = d2PI * glideFromHz / sampleRate;
			mGlideRemaining = n;
			// The ratio that gets from here to there in exactly n steps.
			// Equal ratios per sample is equal SEMITONES per sample, which
			// is why an octave down sweeps as evenly as an octave up.
			mGlideRatio     = std::pow (frequency / glideFromHz, 1.0 / static_cast<double> (n));
		}
	}

	// -- LFOs ------------------------------------------------------------
	mLfoThetaVibrato = 0.0;
	mLfoStepVibrato  = lfoStep (params.vibratoRate, sampleRate);
	// DXi: depth 0 means off; otherwise 2*pi / ((1 - depth) * sampleRate).
	mLfoDepthVibrato = (params.vibratoDepth != 0.f)
	                       ? lfoStep (1.0 - params.vibratoDepth, sampleRate)
	                       : 0.0;

	mLfoThetaTremolo = 0.0;
	mLfoStepTremolo  = lfoStep (params.tremoloRate, sampleRate);
	mLfoDepthTremolo = params.tremoloDepth;

	// -- partials --------------------------------------------------------
	mHarmLfoActive = false;
	for (int i = 0; i < kNumPartials; ++i)
	{
		mHarmLevel[i] = params.harmInvert[i] ? -params.harmLevel[i] : params.harmLevel[i];
		// DXi: fPhaseSliders = dPIm * (0.5 - control). Note the negative pi.
		mHarmPhase[i] = static_cast<float> (dPIm * (0.5 - params.harmPhase[i]));

		// The per-harmonic LFO. Theta starts at zero as the DXi's
		// dFormantsLFOTheta[i-1]=0.0 did, and sin(0) is 0, so the multiplier
		// starts at exactly 1 and a note cannot begin with a step.
		mHarmLfoTheta[i] = 0.0;
		mHarmLfoStep[i]  = d2PI * harmonicLfoHz (params.harmLfoRate[i]) / sampleRate;
		mHarmLfoDepth[i] = params.harmLfoDepth[i];
		if (mHarmLfoDepth[i] != 0.f)
			mHarmLfoActive = true;

		Adsr& e = mFormantEnvelope[i];
		e.reset ();
		e.setSampleRate (sampleRate);
		e.setAttackTime (params.formantAttack[i]);
		e.setDecayTime (params.formantDecay[i]);
		e.setSustainLevel (params.formantSustain[i]);
		e.setReleaseTime (params.formantRelease[i]);
		e.setLevel (params.formantLevel[i]);
		e.setEnabled (params.formantEnable[i]);
		e.setInvert (params.formantInvert[i]);
	}

	// -- envelopes -------------------------------------------------------
	mAmpEnvelope.reset ();
	mAmpEnvelope.setSampleRate (sampleRate);
	mAmpEnvelope.setAttackTime (params.envAttack);
	mAmpEnvelope.setDecayTime (params.envDecay);
	mAmpEnvelope.setSustainLevel (params.envSustain);
	mAmpEnvelope.setReleaseTime (params.envRelease);
	mAmpEnvelope.setLevel (params.envLevel);
	mAmpEnvelope.setEnabled (true);      // the DXi always enabled this one
	mAmpEnvelope.setInvert (false);

	mFilterEnvelope.reset ();
	mFilterEnvelope.setSampleRate (sampleRate);
	mFilterEnvelope.setAttackTime (params.filterAttack);
	mFilterEnvelope.setDecayTime (params.filterDecay);
	mFilterEnvelope.setSustainLevel (params.filterSustain);
	mFilterEnvelope.setReleaseTime (params.filterRelease);
	mFilterEnvelope.setLevel (params.filterLevel);
	mFilterEnvelope.setEnabled (true);
	mFilterEnvelope.setInvert (false);

	mShapes = params.shapes;

	// -- level and pan ---------------------------------------------------
	mVelocity = std::clamp (velocity, 0, 127);
	mBendFactor = 1.0;
	setChannelVolume (channelVolume);
	setPan (pan);

	// A fresh ladder per voice - see the header.
	mIn1 = mIn2 = mIn3 = mIn4 = 0.0;
	mOut1 = mOut2 = mOut3 = mOut4 = 0.0;
}

//------------------------------------------------------------------------
void Voice::setChannelVolume (int volume)
{
	mChannelVolume = std::clamp (volume, 0, 127);
	// DXi: sqrt(cc7 * velocity / (127*127)) * 0.25 - "allow for 12dB of
	// headroom", its comment.
	mVolume = static_cast<float> (
	    std::sqrt (static_cast<double> (mChannelVolume * mVelocity) / (127.0 * 127.0)) * 0.25);
}

//------------------------------------------------------------------------
void Voice::setPan (int pan)
{
	// panToLRGain. The DXi's table is
	//     dx = n/128,  L = sqrt(2(1-dx)),  R = sqrt(2 dx)
	// for n 0..64, mirrored above centre.
	//
	// Its TAPER is not the odd part - it is a textbook constant-power law,
	// L^2 + R^2 = 2.000 at every position, so a note is equally loud
	// wherever it sits. What is unconventional is where it is normalised:
	// the DXi puts unity at the CENTRE, which leaves a hard-panned note
	// peaking 3.01 dB higher than a centred one.
	//
	// DELIBERATE DEVIATION: divided by sqrt(2), which is the convention -
	// unity at the EXTREMES, centre 3 dB down, so panning can never add
	// peak level. The taper is untouched; it was already right. Everything
	// is therefore 3.01 dB quieter than the DXi at every pan position, which
	// the output trim covers. See PORTING-NOTES section 7.
	int n = std::clamp (pan, 0, 127);
	n = (n * 128) / 127;

	auto tableL = [] (int i) { return std::sqrt (1.0 - i / 128.0); };
	auto tableR = [] (int i) { return std::sqrt (i / 128.0); };

	if (n <= 64)
	{
		mVolL = static_cast<float> (tableL (n));
		mVolR = static_cast<float> (tableR (n));
	}
	else
	{
		mVolL = static_cast<float> (tableR (128 - n));
		mVolR = static_cast<float> (tableL (128 - n));
	}
}

//------------------------------------------------------------------------
void Voice::noteOff ()
{
	if (mReleased)
		return;
	mAmpEnvelope.noteOff ();
	mFilterEnvelope.noteOff ();
	for (int i = 0; i < kNumPartials; ++i)
		mFormantEnvelope[i].noteOff ();
	mReleased = true;
}

//------------------------------------------------------------------------
float Voice::renderSample (const FilterInfo& filter)
{
	const float tremolo = static_cast<float> (std::sin (mLfoThetaTremolo)) *
	                      static_cast<float> (mLfoDepthTremolo);
	const float vibrato = static_cast<float> (std::sin (mLfoThetaVibrato)) *
	                      static_cast<float> (mLfoDepthVibrato);

	// The filter envelope sweeps the lookup bin downwards from 440 Hz as it
	// opens. See kFilterSweepBins in Voice.h for the one change from the DXi.
	//
	// The DXi truncated this to an integer bin, so a sweep stepped through
	// the response a quarter-semitone at a time and every partial's
	// magnitude and phase changed abruptly hundreds of times a note. The bin
	// is fractional here and the response is interpolated between the two
	// bins either side of it. See PORTING-NOTES section 6.
	double binF = kFilterBinOrigin - static_cast<double> (kFilterSweepBins)
	                                     * mFilterEnvelope.nextValue ();
	// The envelope overshoots 1.0 by one sample, which is the only way this
	// goes negative now that the sweep spans exactly the bins that exist.
	// The DXi clamped that one sample to bin 1; clamping to 0 keeps the
	// bottom of the sweep where the rest of the sweep says it is.
	if (binF < 0.0) binF = 0.0;
	const double topBin = FilterInfo::kNumBins - 1;
	if (binF > topBin) binF = topBin;

	int lower = static_cast<int> (binF);
	if (lower > FilterInfo::kNumBins - 2)
		lower = FilterInfo::kNumBins - 2;
	const float frac = static_cast<float> (binF - lower);

	if (lower != mFilterCacheBin)
	{
		float m0[kNumPartials], t0[kNumPartials];
		float m1[kNumPartials], t1[kNumPartials];
		filter.combine (mShapes, lower,     m0, t0);
		filter.combine (mShapes, lower + 1, m1, t1);
		for (int k = 0; k < kNumPartials; ++k)
		{
			mFilterMag[k]   = m0[k];
			mFilterTheta[k] = t0[k];
			mFilterDMag[k]  = m1[k] - m0[k];
			// Round the short way. Interpolating +3.1 to -3.1 the long way
			// would sweep a partial right through the opposite phase, which
			// is the very thing this is here to stop.
			float d = t1[k] - t0[k];
			while (d >  static_cast<float> (dPI)) d -= static_cast<float> (d2PI);
			while (d < -static_cast<float> (dPI)) d += static_cast<float> (d2PI);
			mFilterDTheta[k] = d;
		}
		mFilterCacheBin = lower;
	}

	float magnitude[kNumPartials];
	float theta[kNumPartials];
	for (int k = 0; k < kNumPartials; ++k)
	{
		magnitude[k] = mFilterMag[k]   + frac * mFilterDMag[k];
		theta[k]     = mFilterTheta[k] + frac * mFilterDTheta[k];
	}

	// Ten partials at k*theta. dFormantLFO in the original is a constant
	// 1.0 - the per-formant LFO was initialised at note-on and never read -
	// so it is not carried over.
	double sum = 0.0;
	if (!mHarmLfoActive)
	{
		for (int k = 0; k < kNumPartials; ++k)
		{
			const double partialTheta = (k + 1) * mTheta + theta[k] + mHarmPhase[k];
			sum += mFormantEnvelope[k].nextValue () * std::sin (partialTheta)
			       * mHarmLevel[k] * magnitude[k];
		}
	}
	else
	{
		// The DXi threaded a `dFormantLFO` multiplier through all ten of
		// these terms and then never assigned it anything but 1.0. This is
		// that multiplier, finally driven - per harmonic, as the three dead
		// arrays it allocated per note always intended. It sits OUTSIDE the
		// sin(), so it scales amplitude, which is what a depth of 3.0/i in
		// the original could only have meant.
		for (int k = 0; k < kNumPartials; ++k)
		{
			const double partialTheta = (k + 1) * mTheta + theta[k] + mHarmPhase[k];
			const double lfo = (mHarmLfoDepth[k] != 0.f)
			                       ? 1.0 + mHarmLfoDepth[k] * std::sin (mHarmLfoTheta[k])
			                       : 1.0;
			sum += lfo * mFormantEnvelope[k].nextValue () * std::sin (partialTheta)
			       * mHarmLevel[k] * magnitude[k];
			mHarmLfoTheta[k] += mHarmLfoStep[k];
		}
	}

	const float envVolume = mAmpEnvelope.nextValue ();
	double samp = envVolume * (1.0 + tremolo) * mVolume * sum;

	// -- the fixed Moog ladder, per voice --------------------------------
	if (mLadderEnabled)
	{
		samp -= mOut4 * kLadderFb;
		samp *= kLadderFf;
		mOut1 = samp  + 0.3 * mIn1 + (1.0 - kLadderF) * mOut1;   mIn1 = samp;
		mOut2 = mOut1 + 0.3 * mIn2 + (1.0 - kLadderF) * mOut2;   mIn2 = mOut1;
		mOut3 = mOut2 + 0.3 * mIn3 + (1.0 - kLadderF) * mOut3;   mIn3 = mOut2;
		mOut4 = mOut3 + 0.3 * mIn4 + (1.0 - kLadderF) * mOut4;   mIn4 = mOut3;

		samp = mOut4 * 5.0;
	}
	else
	{
		// Level-matched bypass - see setLadderEnabled in Voice.h. The state
		// is run down to zero rather than frozen, so switching the ladder
		// back in mid-note starts from silence in its delays instead of
		// from whatever was left in them, which would be a click.
		samp *= kLadderDcGain;
		mIn1 = mIn2 = mIn3 = mIn4 = 0.0;
		mOut1 = mOut2 = mOut3 = mOut4 = 0.0;
	}

	// Advance. Vibrato is added to the phase step, as in the original.
	mLfoThetaTremolo += mLfoStepTremolo;
	mLfoThetaVibrato += mLfoStepVibrato;
	// DXi: dStep = pns->dStep * m_aMidiChan[chan].fWheelPitch, read fresh
	// each block, so the wheel bends notes that are already sounding.
	mTheta += (mStep * mBendFactor + vibrato);
	++mCurrentSample;

	// The glide, AFTER the sample it produced: the first sample of a note
	// sounds at the pitch it is gliding from, which is the whole point.
	// The bend multiplies whatever the glide has reached, so bending
	// mid-glide does what you would expect of both.
	if (mGlideRemaining > 0)
	{
		if (--mGlideRemaining == 0)
			mStep = mStepTarget;
		else
			mStep *= mGlideRatio;
	}

	// The DXi had no guard here at all. Feedback is fixed and the envelopes
	// are bounded, so this should never fire - but a NaN reaching the host
	// is the one failure worth spending a branch on.
	if (!std::isfinite (samp))
	{
		mIn1 = mIn2 = mIn3 = mIn4 = 0.0;
		mOut1 = mOut2 = mOut3 = mOut4 = 0.0;
		return 0.f;
	}
	return static_cast<float> (samp);
}

//------------------------------------------------------------------------
void Voice::render (float* stereo, int numSamples, const FilterInfo& filter)
{
	if (!mActive)
		return;

	// The filter tables are rebuilt between blocks when a control has moved,
	// and a voice is never told. Dropping the cache once per block is the
	// cheapest way to be certain it never draws on a table that has gone.
	mFilterCacheBin = -1;

	for (int i = 0; i < numSamples; ++i)
	{
		const float samp = renderSample (filter);
		stereo[2 * i]     += mVolL * samp;
		stereo[2 * i + 1] += mVolR * samp;
	}

	// A released voice is finished once its amplitude envelope has run out.
	if (mReleased && mAmpEnvelope.isFinished ())
		mActive = false;
}

//------------------------------------------------------------------------
} // namespace ForTran
