//------------------------------------------------------------------------
// ForTran - ADSR envelope implementation
//------------------------------------------------------------------------

#include "Adsr.h"

#include <algorithm>
#include <cmath>

namespace ForTran {

namespace {
/** The DXi divided by its attack/decay/release counts with no guard, and
    those counts come straight from controls whose minimum is zero, so a
    control at the bottom of its travel produced NaN on the very first
    sample and poisoned the note.
 
    The floor is one **millisecond**, not one sample. One sample is not
    enough: the phase tests fire a sample late, so `value` overshoots by
    1/count before the phase changes - harmlessly at 1.0001 when the count
    is in the thousands, but reaching 2.0 with a count of 1, and then
    leaving DECAY at -1.0 which SUSTAIN holds for the life of the note. A
    millisecond bounds the overshoot at about 1.02.
 
    A millisecond is below anything reachable in practice: the control is a
    percentage and 1 % is already 10 ms, so this only engages in the bottom
    0.1 % of its travel. Everywhere else the count is unchanged and the
    envelope is bit-identical to the original - asserted in
    tests/AdsrTests.cpp against a transcription of it. */
inline long atLeastOneMillisecond (double samples, double sampleRate)
{
	const double floorSamples = sampleRate / 1000.0;
	if (!(samples > floorSamples))       // also catches NaN
		return static_cast<long> (floorSamples > 1.0 ? floorSamples : 1.0);
	return static_cast<long> (samples);
}
/** The control, shaped. See the TIME RANGE note in Adsr.h: squaring the
    0..1 control before scaling widens the top of the travel without moving
    the factory 20 % setting off the DXi's original timing. */
inline double shaped (float rate)
{
	const double r = (rate > 0.0f) ? ((rate < 1.0f) ? static_cast<double> (rate) : 1.0) : 0.0;
	return r * r;
}
} // anonymous namespace

//------------------------------------------------------------------------
void Adsr::setSampleRate (double sampleRate)
{
	mSampleRate = (sampleRate > 1000.0) ? sampleRate : 44100.0;
}

//------------------------------------------------------------------------
void Adsr::setAttackTime (float rate)
{
	mAttack = atLeastOneMillisecond (shaped (rate) * kMaxAttackDecaySeconds * mSampleRate,
	                                 mSampleRate);
}

void Adsr::setDecayTime (float rate)
{
	mDecay = atLeastOneMillisecond (shaped (rate) * kMaxAttackDecaySeconds * mSampleRate,
	                                mSampleRate);
}

void Adsr::setSustainLevel (float rate)
{
	mSustain = rate;
	// The original set this here too. SetRelease() overwrites it, so it only
	// matters for a note released before it has ever sounded.
	mReleaseInitial = rate;
}

void Adsr::setReleaseTime (float rate)
{
	// Note the 2:1 against attack: release is twice as long for the same
	// control value. That asymmetry is the original's, and widening kept it
	// (kMaxReleaseSeconds is twice kMaxAttackDecaySeconds).
	mRelease = atLeastOneMillisecond (shaped (rate) * kMaxReleaseSeconds * mSampleRate,
	                                  mSampleRate);
}

//------------------------------------------------------------------------
void Adsr::reset ()
{
	mPhase = Phase::Attack;
	mValue = 0.f;
	mCurrentSample = 0;
	mSustainSample = 0;
	mReleaseInitial = 0.f;
}

//------------------------------------------------------------------------
void Adsr::noteOff ()
{
	if (mPhase != Phase::Release)
	{
		mSustainSample  = mCurrentSample;
		mReleaseInitial = mValue;
	}
	mPhase = Phase::Release;
}

//------------------------------------------------------------------------
float Adsr::nextValue ()
{
	float value = mValue;

	switch (mPhase)
	{
		case Phase::Attack:
			value = static_cast<float> (mCurrentSample) / static_cast<float> (mAttack);
			if (mCurrentSample > mAttack || value > 1.f)
				mPhase = Phase::Decay;
			break;

		case Phase::Decay:
			value = 1.f - ((static_cast<float> (mCurrentSample) - static_cast<float> (mAttack))
			               / static_cast<float> (mDecay));
			if (value <= mSustain)
				mPhase = Phase::Sustain;
			break;

		case Phase::Sustain:
			// Deliberately empty, as in the original: the value stays where
			// decay left it, which is at or just below the sustain level.
			break;

		case Phase::Release:
			value = mReleaseInitial - ((static_cast<float> (mCurrentSample)
			                            - static_cast<float> (mSustainSample))
			                           / static_cast<float> (mRelease));
			if (value < 0.f)
				value = 0.f;
			break;
	}

	mValue = value;
	++mCurrentSample;

	if (!mEnabled)
		return 1.f;
	if (mInvert)
		return mLevel * (1.f - value);
	return mLevel * value;
}

//------------------------------------------------------------------------
// traceEnvelope
//------------------------------------------------------------------------
namespace {

/** The rate the plot is traced at. Not the host's: a 10 s release at 96 kHz
    is a million samples to walk on every knob movement, and the picture is
    two hundred pixels wide. 4 kHz keeps the longest envelope under 150k
    steps while still being fine enough that the millisecond floor is four
    samples rather than none - the shortest settings still draw correctly. */
constexpr double kTraceRate = 4000.0;

/** Nothing here can run away: a runaway loop in the editor would hang the
    host's UI thread. 40 s is well past the 15 s an envelope can reach. */
constexpr long kTraceCap = static_cast<long> (kTraceRate * 40.0);

void configure (Adsr& e, const EnvelopeSpec& spec)
{
	e.reset ();
	e.setSampleRate (kTraceRate);
	e.setAttackTime (spec.attack);
	e.setDecayTime (spec.decay);
	e.setSustainLevel (spec.sustain);
	e.setReleaseTime (spec.release);
	e.setLevel (spec.level);
	e.setEnabled (spec.enabled);
	e.setInvert (spec.invert);
}

} // anonymous namespace

//------------------------------------------------------------------------
double traceEnvelope (const EnvelopeSpec& spec, float* out, int count, double* keyUpFraction)
{
	if (keyUpFraction)
		*keyUpFraction = -1.0;
	if (!out || count < 2)
		return 0.0;

	// A disabled envelope is not a shape. `nextValue` returns a flat 1.0 for
	// it - ignoring level and invert - so that is what is drawn, and there
	// is no duration to report.
	if (!spec.enabled)
	{
		for (int i = 0; i < count; ++i)
			out[i] = 1.f;
		return 0.0;
	}

	// -- pass one: how long is it? ---------------------------------------
	long toSustain = 0;
	{
		Adsr e;
		configure (e, spec);
		while (toSustain < kTraceCap && e.phase () != Adsr::Phase::Sustain)
		{
			e.nextValue ();
			++toSustain;
		}
	}

	long hold = toSustain / 4;
	const long minimumHold = static_cast<long> (kTraceRate * 0.005);
	if (hold < minimumHold)
		hold = minimumHold;

	long releaseLength = 0;
	{
		Adsr e;
		configure (e, spec);
		for (long i = 0; i < toSustain + hold; ++i)
			e.nextValue ();
		e.noteOff ();
		// isFinished() tests the envelope's own value, not what nextValue
		// returns - which matters, because a level of zero or an inverted
		// envelope would otherwise look finished immediately, or never.
		while (releaseLength < kTraceCap && !e.isFinished ())
		{
			e.nextValue ();
			++releaseLength;
		}
	}

	const long total = toSustain + hold + releaseLength;
	if (total < 2)
	{
		for (int i = 0; i < count; ++i)
			out[i] = 0.f;
		return 0.0;
	}

	if (keyUpFraction)
		*keyUpFraction = static_cast<double> (toSustain + hold) / static_cast<double> (total - 1);

	// -- pass two: emit `count` evenly spaced points ---------------------
	Adsr e;
	configure (e, spec);
	int written = 0;
	for (long n = 0; n < total && written < count; ++n)
	{
		if (n == toSustain + hold)
			e.noteOff ();
		float v = e.nextValue ();

		// The ONE thing the plot does not reproduce: the envelope's
		// one-sample overshoot past full level. It is real - the phase test
		// fires after the value is computed - but its size is 1/attack, so
		// at the host's 44.1 kHz it is 1.02 and 23 microseconds long, while
		// at the trace's 4 kHz it would draw as 1.25. Plotting it here would
		// not be faithful, it would be an artefact of the plot's own
		// resolution, ten times the size of the thing it claims to show. So
		// the trace is clamped to the range the envelope legitimately
		// occupies. tests/AdsrTests.cpp asserts the overshoot itself
		// separately, against the 2003 transcription, where it belongs.
		const float ceiling = (spec.level > 0.f) ? spec.level : 0.f;
		if (v > ceiling) v = ceiling;
		if (v < 0.f)     v = 0.f;

		// One sample can cover several output points when the envelope is
		// short, and several samples one point when it is long.
		while (written < count &&
		       static_cast<long> ((static_cast<double> (written) * (total - 1))
		                          / (count - 1) + 0.5) == n)
			out[written++] = v;
	}
	// Rounding can leave the last point or two unwritten; hold the last
	// value rather than leaving them uninitialised.
	const float tail = (written > 0) ? out[written - 1] : 0.f;
	while (written < count)
		out[written++] = tail;

	return static_cast<double> (total) / kTraceRate;
}

//------------------------------------------------------------------------
} // namespace ForTran
