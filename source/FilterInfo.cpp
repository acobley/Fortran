//------------------------------------------------------------------------
// ForTran - filter response tables implementation
//------------------------------------------------------------------------

#include "FilterInfo.h"

#include <algorithm>
#include <cmath>

namespace ForTran {

namespace {
constexpr double dPI = 3.14159265358979323846264338327950288419716939937510;

/** The phase of the filter's complex response.

    DELIBERATE DEVIATION. The DXi wrote `atan (RI / RR)` (ForTran.cpp:87 and
    :99). A ratio has thrown its quadrant away before atan sees it, so atan
    can only answer in -pi/2..+pi/2: wherever the real part changes sign the
    reported phase jumps by pi although the true phase is perfectly smooth.

    And the real part does change sign, repeatedly - it is
    `feedback + radius^poles * cos (poles * theta)`, which oscillates as the
    pole count rises. At six poles a partial at FULL magnitude reverses its
    sign between two adjacent bins, and there are dozens of such bins across
    the range the filter envelope sweeps. Audible as clicks, and the reason
    they only surfaced once the sweep was fixed to cover the whole envelope
    (see PORTING-NOTES section 7) instead of being pinned at the bottom of
    the table where the attack transient masked them.

    atan2 returns the true angle. Its only discontinuity is the wrap at
    +-pi, which is a 2*pi step and therefore invisible to sin(). The zero
    guard is kept: the DXi's 0/0 was a NaN that propagated into every
    partial, and atan2 (0, 0) is 0 on IEEE hardware but the guard says so
    rather than relying on it.

    tests/FilterInfoTests.cpp pins the size of the change: magnitudes stay
    bit-identical to the transcription of the original, and every phase is
    either identical to it or differs by exactly pi - which is the whole of
    what a quadrant is. */
inline float filterPhase (double ri, double rr)
{
	if (rr == 0.0 && ri == 0.0)
		return 0.f;
	return static_cast<float> (std::atan2 (ri, rr));
}

/** The ten harmonics, as offsets on the 1/48-octave grid: harmonic n sits
    12*log2(n) semitones up, rounded to the nearest semitone and multiplied
    by 4 because the grid is a quarter-semitone. Verbatim from the DXi's
    Values[] - the rounding is part of the sound. */
constexpr int kHarmonicOffset[FilterInfo::kNumParts] =
    { 0, 12 * 4, 19 * 4, 24 * 4, 28 * 4, 31 * 4, 34 * 4, 36 * 4, 38 * 4, 40 * 4 };
} // anonymous namespace

//------------------------------------------------------------------------
FilterInfo::FilterInfo ()
{
	// A 1/48-octave grid with bin 69*4 = 276 pinned to 440 Hz, filled
	// outwards. Spans about 8 Hz to 23 kHz.
	const double step = std::pow (2.0, 1.0 / 48.0);

	mFrequency[69 * 4] = 440.f;
	for (int ix = 69 * 4 + 1; ix < kNumBins; ++ix)
		mFrequency[ix] = static_cast<float> (mFrequency[ix - 1] * step);
	for (int ix = 69 * 4 - 1; ix >= 0; --ix)
		mFrequency[ix] = static_cast<float> (mFrequency[ix + 1] / step);

	mFeedback = 0.5f;
	mPoles    = 4.f;
	mCutoff   = mFrequency[69 * 4];
	calculate ();
}

//------------------------------------------------------------------------
void FilterInfo::setFeedback (float feedback)
{
	if (feedback != mFeedback) { mFeedback = feedback; mDirty = true; }
}

void FilterInfo::setPoles (float poles)
{
	// The DXi truncated to int before storing.
	const float p = static_cast<float> (static_cast<int> (poles));
	if (p != mPoles) { mPoles = p; mDirty = true; }
}

void FilterInfo::setCutoff (float cutoff01)
{
	// DXi: Cutoff = Frequencys[(int)(fCutoff * 552.0)].
	//
	// At the top of the control fCutoff is exactly 1.0, so that index is 552
	// - one past the end of a 552-element array. Clamped here. Everywhere
	// below the very top of the travel the index is unchanged.
	int bin = static_cast<int> (cutoff01 * static_cast<double> (kNumBins));
	bin = std::clamp (bin, 0, kNumBins - 1);

	const float c = mFrequency[bin];
	if (c != mCutoff) { mCutoff = c; mDirty = true; }
}

//------------------------------------------------------------------------
void FilterInfo::calculate ()
{
	const double feedback = mFeedback;
	const double poles    = mPoles;
	// Kept as float, deliberately. The original divided two floats and only
	// then widened the result, so the ratios carry float precision; doing
	// the division in double instead moves the tables by ~6e-8, which the
	// bin-for-bin comparison in tests/FilterInfoTests.cpp picks up.
	const float  cutoff   = (mCutoff > 0.f) ? mCutoff : 1.f;

	// CombPoles = 2.5/10 * Poles in the original; dDivPoles was computed and
	// never used, its line having been commented out above it.
	const double combPoles = 2.5 / 10.0 * poles;

	for (int ix = 0; ix < kNumBins; ++ix)
	{
		const float freq = mFrequency[ix];

		// -- low pass ----------------------------------------------------
		{
			const double ratio  = freq / cutoff;      // float division, then widened
			const double radius = std::sqrt (1.0 + ratio * ratio);
			const double theta  = std::atan (ratio);
			const double pr     = std::pow (radius, poles) * std::cos (poles * theta);
			const double pi_    = std::pow (radius, poles) * std::sin (poles * theta);
			const double kr     = feedback + pr;
			const double den    = kr * kr + pi_ * pi_;
			const double rr     = (den != 0.0) ? kr / den : 0.0;
			const double ri     = (den != 0.0) ? -pi_ / den : 0.0;

			float mag = static_cast<float> (std::sqrt (rr * rr + ri * ri));
			if (!(mag <= 1.f)) mag = 1.f;          // the DXi's clamp; also traps NaN
			mLowPassMagnitude[ix] = mag;
			mLowPassTheta[ix]     = filterPhase (ri, rr);
		}

		// -- high pass ---------------------------------------------------
		{
			const double ratio  = cutoff / freq;      // float division, then widened
			const double radius = std::sqrt (1.0 + ratio * ratio);
			const double theta  = std::atan (ratio);
			const double pr     = std::pow (radius, poles) * std::cos (poles * theta);
			const double pi_    = std::pow (radius, poles) * std::sin (poles * theta);
			const double kr     = feedback + pr;
			const double den    = kr * kr + pi_ * pi_;
			const double rr     = (den != 0.0) ? kr / den : 0.0;
			const double ri     = (den != 0.0) ? -pi_ / den : 0.0;

			float mag = static_cast<float> (std::sqrt (rr * rr + ri * ri));
			if (!(mag <= 1.f)) mag = 1.f;
			mHighPassMagnitude[ix] = mag;
			mHighPassTheta[ix]     = filterPhase (ri, rr);
		}

		// -- comb and grass ----------------------------------------------
		//
		// Note the comb is indexed by BIN, not by frequency: the original's
		// frequency-based line is commented out just above it and this one
		// replaced it. So the comb's teeth are evenly spaced in pitch, not
		// in Hz, which is what makes it sound like a comb over a harmonic
		// series rather than over a spectrum. Kept.
		{
			const double combRatio = static_cast<double> (ix) / 48.0;
			const double rawSin    = std::sin (dPI * combRatio * combPoles);
			const double absSin    = std::fabs (rawSin);

			float comb = static_cast<float> (std::pow (absSin, feedback / 4.0));
			if (!(comb <= 1.f)) comb = 1.f;
			mCombMagnitude[ix] = comb;
			mCombTheta[ix]     = static_cast<float> (dPI * rawSin);

			float grass = 1.f - comb;
			if (!(grass <= 1.f)) grass = 1.f;
			mGrassMagnitude[ix] = grass;
			mGrassTheta[ix]     = static_cast<float> (dPI * rawSin);
		}
	}

	mDirty = false;
}

//------------------------------------------------------------------------
void FilterInfo::values (Shape shape, int bin, float* magnitude, float* theta) const
{
	const float* mag = nullptr;
	const float* th  = nullptr;
	switch (shape)
	{
		case Shape::LowPass:  mag = mLowPassMagnitude;  th = mLowPassTheta;  break;
		case Shape::HighPass: mag = mHighPassMagnitude; th = mHighPassTheta; break;
		case Shape::Comb:     mag = mCombMagnitude;     th = mCombTheta;     break;
		case Shape::Grass:    mag = mGrassMagnitude;    th = mGrassTheta;    break;
	}

	for (int jx = 0; jx < kNumParts; ++jx)
	{
		// The DXi guarded only the low-pass lookup, and with `> 552` rather
		// than `>= 552`, so it could read one element past the end; the other
		// three shapes had no guard at all. In the shipped plug-in this was
		// harmless - the caller clamps its index to at most 276 and the
		// largest harmonic offset is 160 - but the guard belongs here, where
		// it cannot be undone by a change to the caller.
		const int offset = std::clamp (bin + kHarmonicOffset[jx], 0, kNumBins - 1);
		magnitude[jx] = mag[offset];
		theta[jx]     = th[offset];
	}
}

//------------------------------------------------------------------------
// The same fold as combine(), applied to every bin rather than to ten
// harmonics. Magnitude only - the response panel has no phase axis.
//
// The DXi drew this from a SECOND implementation living in its property
// page (ForTranPropPage.cpp:905-981), which had a dangling-else bug of its
// own: `case 3:` there reads as "if LP on, assign unconditionally; if LP
// off, do nothing" - the inverse of its indentation. So the graph the user
// saw did not match the audio for Comb or Grass set to Max. One
// implementation, shared, removes the whole category of problem.
//------------------------------------------------------------------------
void FilterInfo::combinedCurve (const ShapeStates& states, float* out) const
{
	bool written = false;

	if (states.lowPass != 0)
	{
		for (int i = 0; i < kNumBins; ++i)
			out[i] = mLowPassMagnitude[i];
		written = true;
	}
	else
	{
		for (int i = 0; i < kNumBins; ++i)
			out[i] = 1.f;
	}

	const Shape order[3] = { Shape::HighPass, Shape::Comb, Shape::Grass };
	const int   state[3] = { states.highPass, states.comb, states.grass };

	for (int stage = 0; stage < 3; ++stage)
	{
		if (state[stage] == static_cast<int> (Mode::Off))
			continue;

		const float* m = magnitudeCurve (order[stage]);

		switch (static_cast<Mode> (state[stage]))
		{
			case Mode::Add:
				for (int i = 0; i < kNumBins; ++i)
					out[i] = written ? out[i] + m[i] : m[i];
				break;
			case Mode::Mult:
				for (int i = 0; i < kNumBins; ++i)
					out[i] *= m[i];
				break;
			case Mode::Max:
				for (int i = 0; i < kNumBins; ++i)
					out[i] = written ? std::max (out[i], m[i]) : m[i];
				break;
			case Mode::Off:
			default:
				break;
		}
		written = true;
	}
}

//------------------------------------------------------------------------
const float* FilterInfo::magnitudeCurve (Shape shape) const
{
	switch (shape)
	{
		case Shape::LowPass:  return mLowPassMagnitude;
		case Shape::HighPass: return mHighPassMagnitude;
		case Shape::Comb:     return mCombMagnitude;
		case Shape::Grass:    return mGrassMagnitude;
	}
	return mLowPassMagnitude;
}

//------------------------------------------------------------------------
float FilterInfo::binFrequency (int bin) const
{
	return mFrequency[std::clamp (bin, 0, kNumBins - 1)];
}

//------------------------------------------------------------------------
// Folding the four shapes together.
//
// The DXi did this inline in synthNote, 600 lines of hand-unrolled switch
// with each of the ten elements written out. Three things are corrected
// here; PORTING-NOTES section 6 has the full account.
//
//  * Max works. All three Max branches computed max() into the shape's own
//    scratch array and never copied it back - the line that would have is
//    commented out directly below each one - so Max did nothing at all when
//    low pass was on, and threw away everything before it when it was off.
//
//  * "Has anything contributed yet?" is a flag. The original asked it by
//    naming particular predecessors, so Grass=Add tested low pass and high
//    pass but never comb, and silently discarded the comb contribution.
//
//  * Mult and Max leave the phase alone. Multiplying two angles is not
//    meaningful, and with low pass off the phase accumulator starts at zero,
//    so a single Mult stage annihilated the phase for good. Add still
//    accumulates phase, which is both meaningful and what the original did.
//------------------------------------------------------------------------
void FilterInfo::combine (const ShapeStates& states, int bin,
                          float* magnitude, float* theta) const
{
	bool written = false;

	// Stage 0: low pass, which is a plain Off/On switch and always runs -
	// it is what initialises both arrays. The original relied on this too:
	// its else branch is the only unconditional write in the whole region,
	// so turning it into a switch with a default would leave the arrays
	// uninitialised.
	if (states.lowPass != 0)
	{
		values (Shape::LowPass, bin, magnitude, theta);
		written = true;
	}
	else
	{
		for (int i = 0; i < kNumParts; ++i)
		{
			magnitude[i] = 1.f;   // multiplicative identity
			theta[i]     = 0.f;   // additive identity
		}
	}

	const Shape order[3] = { Shape::HighPass, Shape::Comb, Shape::Grass };
	const int   state[3] = { states.highPass, states.comb, states.grass };

	for (int stage = 0; stage < 3; ++stage)
	{
		if (state[stage] == static_cast<int> (Mode::Off))
			continue;

		float m[kNumParts];
		float t[kNumParts];
		values (order[stage], bin, m, t);

		switch (static_cast<Mode> (state[stage]))
		{
			case Mode::Add:
				for (int i = 0; i < kNumParts; ++i)
				{
					theta[i]    += t[i];
					magnitude[i] = written ? magnitude[i] + m[i] : m[i];
				}
				break;

			case Mode::Mult:
				for (int i = 0; i < kNumParts; ++i)
					magnitude[i] *= m[i];
				break;

			case Mode::Max:
				for (int i = 0; i < kNumParts; ++i)
					magnitude[i] = written ? std::max (magnitude[i], m[i]) : m[i];
				break;

			case Mode::Off:
			default:
				break;
		}

		written = true;
	}
}

//------------------------------------------------------------------------
} // namespace ForTran
