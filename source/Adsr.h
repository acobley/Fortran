//------------------------------------------------------------------------
// ForTran - ADSR envelope
//
// Line-by-line port of the DXi's ADSR class (ADSR.cpp/.h, 2003), to
// portable C++17 with no SDK types, so it compiles and can be tested
// standalone. See tests/AdsrTests.cpp.
//
// The shape is unusual and is preserved exactly:
//
//   ATTACK   value = currentSample / attack            (linear 0 -> 1)
//   DECAY    value = 1 - (currentSample - attack) / decay
//   SUSTAIN  value is left at whatever DECAY last produced - the original
//            has `case SUSTAIN: break;` and never assigns the sustain level,
//            so the note holds just *below* it. That is the sound.
//   RELEASE  value = releaseInitial - (currentSample - sustainSample) / release
//
// TIME RANGE - the one deliberate departure from the DXi. The original
// mapped the control linearly onto 0..1 s for attack and decay and 0..2 s
// for release, which is too short for slow pads. The mapping here is
//
//     time = rate^2 * maxSeconds        (5 s attack/decay, 10 s release)
//
// The square law is chosen so the *default* patch is untouched: at the
// factory setting of 20 % the square law gives 0.2^2 * 5 = 0.2 s, exactly
// what the DXi's linear 0.2 * 1 gave, and 0.2^2 * 10 = 0.4 s for release,
// exactly its 0.2 * 2. The original's attack:release asymmetry of 1:2 is
// preserved at every setting. Below 20 % the control is finer than the
// DXi's; above it, longer - up to 5 s and 10 s at the top of the travel.
// See PORTING-NOTES section 4.
//
// Envelopes are configured once, at note-on, exactly as the DXi did - their
// times deliberately do not follow automation during a held note.
//------------------------------------------------------------------------

#pragma once

namespace ForTran {

//------------------------------------------------------------------------
class Adsr
{
public:
	enum class Phase { Attack, Decay, Sustain, Release };

	/** Sample rate must be set before any SetXTime call; the DXi left this
	    member uninitialised and relied on the caller. */
	void  setSampleRate (double sampleRate);

	/** Longest time each phase can be dialled to. See the note above: the
	    control is squared before scaling, so the factory 20 % setting still
	    lands on the DXi's original 0.2 s / 0.4 s. */
	static constexpr double kMaxAttackDecaySeconds = 5.0;
	static constexpr double kMaxReleaseSeconds     = 10.0;

	void  setAttackTime  (float rate);   // rate 0..1 -> 0..5 s  (squared)
	void  setDecayTime   (float rate);   // rate 0..1 -> 0..5 s  (squared)
	void  setSustainLevel (float rate);
	void  setReleaseTime (float rate);   // rate 0..1 -> 0..10 s (squared)

	void  setLevel (float level)   { mLevel = level; }
	void  setEnabled (bool enable) { mEnabled = enable; }
	void  setInvert (bool invert)  { mInvert = invert; }

	/** Advance one sample and return the envelope's contribution. Returns
	    1.0 when the envelope is disabled - as the original did, including
	    still advancing its sample counter. */
	float nextValue ();

	void  noteOff ();
	void  reset ();

	Phase phase () const { return mPhase; }

	/** Segment lengths in samples, as the setters computed them. Exposed so
	    tests can drive a transcription of the original with the identical
	    counts and diff the two sample for sample. */
	long  attackSamples  () const { return mAttack; }
	long  decaySamples   () const { return mDecay; }
	long  releaseSamples () const { return mRelease; }
	bool  isFinished () const { return mPhase == Phase::Release && mValue <= 0.f; }

private:
	// NOTE: the DXi declared these `long` and initialised them from float
	// literals (Attack = 0.0005), so every one of them truncated to 0 and
	// the first GetValue() divided by zero. They are seeded to 1 sample
	// here, and setXTime() floors at 1 - see PORTING-NOTES section 4.
	long  mAttack  = 1;
	long  mDecay   = 1;
	long  mRelease = 1;
	float mSustain = 0.5f;

	Phase mPhase = Phase::Attack;
	float mValue = 0.f;
	long  mCurrentSample = 0;
	long  mSustainSample = 0;
	float mReleaseInitial = 0.f;

	double mSampleRate = 44100.0;
	float  mLevel   = 1.f;
	bool   mEnabled = false;
	bool   mInvert  = false;
};

//------------------------------------------------------------------------
// Drawing an envelope
//
// The editor shows each envelope's shape. Rather than draw an idealised
// trapezoid, it drives the real class above and plots what comes out - so
// the picture carries the quirks the sound has: sustain sitting just BELOW
// its control, the one-sample overshoot past 1.0, the millisecond floor,
// invert, and a disabled envelope reading a flat 1.0. A drawing that agreed
// with the panel but not with the DSP would be worse than none.
//
// The single exception is the one-sample overshoot past full level, which
// is clamped away - at the plot's own resolution it would draw ten times
// larger than it really is. See the note in traceEnvelope.
//------------------------------------------------------------------------

/** One envelope's settings, exactly as `Voice::noteOn` applies them. */
struct EnvelopeSpec
{
	float attack  = 0.2f;
	float decay   = 0.2f;
	float sustain = 0.75f;
	float release = 0.2f;
	float level   = 1.f;
	bool  enabled = true;
	bool  invert  = false;
};

/** Plot the envelope into `out` - `count` evenly spaced points spanning
    attack, decay, a short hold at sustain, and release.

    Returns the time in seconds the plot spans, so a caller can label it; a
    disabled envelope has no shape and returns 0 after filling `out` with the
    flat 1.0 that `nextValue` actually produces.

    The hold is a quarter of the time attack and decay took, floored at 5 ms,
    so the sustain level is always visible without a long envelope becoming
    mostly flat line. Everything is traced at a fixed internal rate, so the
    cost does not depend on the host's sample rate. */
double traceEnvelope (const EnvelopeSpec& spec, float* out, int count);

//------------------------------------------------------------------------
} // namespace ForTran
