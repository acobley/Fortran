//------------------------------------------------------------------------
// ForTran - one sounding note
//
// Port of CForTran::synthNote plus the note-on half of InitializeNoteEvent.
// No SDK types, so it builds and runs standalone - see tests/VoiceTests.cpp.
//
// The synthesis, per sample:
//
//   * ten sine partials at k*theta, k = 1..10, each with its own ADSR (the
//     "formant" envelopes), level, phase offset and filter magnitude;
//   * a filter magnitude and phase per partial, looked up from FilterInfo
//     at a bin swept by the filter envelope;
//   * an amplitude envelope, a tremolo LFO on level and a vibrato LFO on
//     the phase step;
//   * a fixed Moog ladder low-pass on the way out, then x5.
//
// The ladder is per-voice here. In the DXi its state lived on the plug-in,
// so every voice was pushed through one filter in sequence - see
// PORTING-NOTES section 7.
//------------------------------------------------------------------------

#pragma once

#include "Adsr.h"
#include "FilterInfo.h"
#include "LiveState.h"

#include <cmath>

namespace ForTran {

//------------------------------------------------------------------------
// The per-harmonic LFO's rate mapping
//
// This is the one definition. The DSP sets its oscillators from it and the
// panel reads its knobs through it, so the number on screen cannot drift
// away from the number in the sound - VST3 puts the processor and the
// controller in separate components, and a private second copy is the
// classic way that goes wrong.
//
// Logarithmic, because a linear Hz knob spends nine tenths of its travel
// above 2 Hz and a linear PERIOD knob spends nine tenths of it below 1 Hz;
// neither is usable for an LFO. Constant ratio per turn instead.
//------------------------------------------------------------------------
constexpr double kHarmLfoMinHz = 0.05;    // one cycle per twenty seconds
constexpr double kHarmLfoMaxHz = 20.0;    // into audio-rate wobble

/** Rate control (0..1) -> Hz. */
inline double harmonicLfoHz (double control)
{
	const double c = (control > 0.0) ? ((control < 1.0) ? control : 1.0) : 0.0;
	// pow, not exp2, so the two constants above can be changed freely.
	return kHarmLfoMinHz * std::pow (kHarmLfoMaxHz / kHarmLfoMinHz, c);
}

/** Hz -> rate control (0..1), clamped to the knob's travel. */
inline double harmonicLfoControl (double hz)
{
	if (!(hz > kHarmLfoMinHz))
		return 0.0;
	if (hz >= kHarmLfoMaxHz)
		return 1.0;
	return std::log (hz / kHarmLfoMinHz) / std::log (kHarmLfoMaxHz / kHarmLfoMinHz);
}

//------------------------------------------------------------------------
/** Everything a voice reads at note-on. The DXi read these from
    CMediaParams inside InitializeNoteEvent; they are latched per note and
    deliberately do not follow automation while the note sounds. */
struct VoiceParams
{
	float harmLevel[10]  = {};   // 0..1, already negated if inverted
	float harmPhase[10]  = {};   // 0..1 from the control
	bool  harmInvert[10] = {};

	float formantAttack[10]  = {};
	float formantDecay[10]   = {};
	float formantSustain[10] = {};
	float formantRelease[10] = {};
	float formantLevel[10]   = {};
	bool  formantEnable[10]  = {};
	bool  formantInvert[10]  = {};

	float envAttack = 0.2f, envDecay = 0.2f, envSustain = 0.75f;
	float envRelease = 0.2f, envLevel = 1.f;

	float filterAttack = 0.2f, filterDecay = 0.2f, filterSustain = 0.75f;
	float filterRelease = 0.2f, filterLevel = 1.f;

	float vibratoRate = 0.2f, vibratoDepth = 0.f;
	float tremoloRate = 0.2f, tremoloDepth = 0.f;

	/** Glide time in SECONDS, already through portamentoSeconds(). Zero is
	    off, and off costs nothing: the voice takes the no-glide path and
	    is sample-identical to one built before this existed. Latched at
	    note-on like everything else here, so changing the knob does not
	    retime a glide already under way. */
	float portamento = 0.f;

	/** The per-harmonic amplitude LFO. Rate is the 0..1 control that
	    harmonicLfoHz() turns into a frequency; depth 0 means off, and off is
	    free - Voice skips the term entirely. */
	float harmLfoRate[10]  = {};
	float harmLfoDepth[10] = {};

	FilterInfo::ShapeStates shapes;
};

//------------------------------------------------------------------------
class Voice
{
public:
	static constexpr int kNumPartials = 10;

	// -- the filter envelope's sweep --------------------------------------
	//
	// The envelope does not set a cutoff; it sweeps the bin the ten
	// partials are read at, downwards from A440.
	//
	//     bin = kFilterBinOrigin - kFilterSweepBins * envelope
	//
	// DXi: `FEnv = 551 * env; Note = ANote - FEnv;` with `ANote = 69*4`.
	// The 551 is the size of the whole table and the 276 is where the sweep
	// starts, and THEY DO NOT MATCH - only the 276 bins below the start are
	// reachable, so the bin hit the bottom at env = 276/551 = 0.5009 and
	// stayed pinned for the rest of the envelope's travel. At the factory
	// settings that silenced the second half of attack, all of decay and
	// all of sustain, and made Filter Level do nothing above 50 %.
	//
	// DELIBERATE DEVIATION: the span is now the range that exists, so the
	// control's full travel maps onto bins 276..0 and Filter Level scales
	// how deep the sweep goes. PORTING-NOTES section 7.
	//
	// The invariant is that these two are equal; VoiceTests asserts it.
	static constexpr int   kFilterBinOrigin = 276;     // ANote = 69*4, A440
	static constexpr float kFilterSweepBins = 276.f;   // the DXi's 551

	/** Start a note. `frequency` is in Hz, `velocity` and `channelVolume`
	    are 0..127, `pan` is 0..127 with 64 centred.

	    `glideFromHz` is where the pitch starts, for portamento - the note
	    the player is coming FROM, which on a polyphonic synth is a
	    question Synth answers (see chooseGlideSource). Zero, or the target
	    itself, or a zero `params.portamento`, all mean no glide and the
	    note simply starts on pitch. */
	void noteOn (int noteNumber, double frequency, int velocity,
	             int channelVolume, int pan,
	             const VoiceParams& params, double sampleRate,
	             double glideFromHz = 0.0);

	void noteOff ();

	/** Channel volume (CC7), pan (CC10) and pitch bend, all 0..127 except
	    the bend factor, which is a frequency multiplier.

	    The DXi re-read all three in synthNote's prologue, which runs once
	    per note PER BLOCK - so they follow the controller live rather than
	    being latched at note-on like everything else. Kept. */
	void setChannelVolume (int volume);
	void setPan (int pan);
	void setPitchBend (double factor) { mBendFactor = factor; }

	/** Switch the output ladder in or out. Pushed every block rather than
	    latched at note-on, unlike almost everything else here: the point of
	    the switch is to A/B it on a note that is already sounding.

	    Bypass is LEVEL-MATCHED. The ladder is not only a filter, it is a
	    fixed +10.6 dB, and a comparison where one side is ten decibels
	    louder tells you nothing about the filtering. With it out, the same
	    DC gain is applied as a plain multiply, so what changes is only what
	    the four poles were doing - which below about B4 is nothing at all.
	    See PORTING-NOTES section 7. */
	void setLadderEnabled (bool enabled) { mLadderEnabled = enabled; }

	/** The harmonic LFOs' rates (0..1 controls) and depths, pushed to a
	    sounding note. Unlike the rest of VoiceParams these FOLLOW the knobs
	    while the note plays: the LFO is this port's own addition, with no
	    DXi behaviour to keep, and an LFO you cannot hear move as you turn
	    it is very hard to set. The phase carries on, so changing the rate
	    does not jump. */
	void setHarmonicLfo (const float* rate, const float* depth);

	/** Add this voice's output into interleaved stereo. */
	void render (float* stereo, int numSamples, const FilterInfo& filter);

	bool isActive () const   { return mActive; }
	int  noteNumber () const { return mNoteNumber; }
	bool isReleased () const { return mReleased; }
	/** Samples rendered so far - the voice stealer's age measure. */
	long age () const { return mCurrentSample; }

	/** What this voice is doing, for the panel. Reads values the last
	    rendered sample left behind; costs nothing to produce. */
	void live (LiveState& out) const;

private:
	float renderSample (const FilterInfo& filter);

	bool  mActive   = false;
	bool  mReleased = false;
	int   mNoteNumber = 0;
	long  mCurrentSample = 0;

	double mTheta = 0.0;
	double mStep  = 0.0;

	// -- portamento --------------------------------------------------------
	//
	// Constant time and exponential in pitch, which together mean one
	// multiply per sample: the step is scaled by a fixed ratio, so equal
	// times give equal intervals and the whole glide takes mGlideRemaining
	// samples whatever the distance. The step is SNAPPED to the target on
	// the last sample rather than left wherever the multiplies landed, so
	// a long glide cannot end a cent flat.
	//
	// Zero portamento leaves mGlideRemaining at 0 and the advance skips the
	// branch entirely - the no-glide path is the old code exactly.
	long   mGlideRemaining = 0;
	double mGlideRatio     = 1.0;
	double mStepTarget     = 0.0;

	double mLfoThetaVibrato = 0.0, mLfoStepVibrato = 0.0, mLfoDepthVibrato = 0.0;
	double mLfoThetaTremolo = 0.0, mLfoStepTremolo = 0.0, mLfoDepthTremolo = 0.0;

	float mHarmLevel[kNumPartials] = {};
	float mHarmPhase[kNumPartials] = {};

	Adsr mAmpEnvelope;
	Adsr mFilterEnvelope;

	// -- the filter lookup, interpolated and cached ------------------------
	//
	// The sweep visits a bin for hundreds of samples at a time, so combine()
	// - four table lookups and a fold, per partial - only has to run when
	// the integer bin changes. What runs every sample is the interpolation
	// between the two cached bins, which is a multiply and an add per
	// partial and no transcendentals at all.
	//
	// The cache is dropped at the top of every render() call, because the
	// filter tables can be rebuilt between blocks and a voice has no way to
	// be told.
	int   mFilterCacheBin = -1;              // -1 = nothing cached
	float mFilterMag[kNumPartials]   = {};   // at the lower bin
	float mFilterTheta[kNumPartials] = {};
	float mFilterDMag[kNumPartials]  = {};   // to the upper bin
	float mFilterDTheta[kNumPartials] = {};  // shortest way round

	/** Ten oscillators, one per partial. `mHarmLfoActive` is the OR of the
	    ten depths, so a patch that uses none pays a single test per sample
	    rather than ten. */
	double mHarmLfoTheta[kNumPartials] = {};
	double mHarmLfoStep[kNumPartials]  = {};
	float  mHarmLfoDepth[kNumPartials] = {};
	bool   mHarmLfoActive = false;
	Adsr mFormantEnvelope[kNumPartials];

	// -- for the panel: the last sample's values, see live() ---------------
	double mSampleRate    = 44100.0;
	long   mReleaseSample = -1;
	float  mLiveAmp       = 0.f;
	float  mLiveFilterEnv = 0.f;
	float  mLiveBin       = static_cast<float> (kFilterBinOrigin);
	float  mLiveFormant[kNumPartials] = {};
	float  mLivePartial[kNumPartials] = {};

	FilterInfo::ShapeStates mShapes;

	float mVolume = 1.f, mVolL = 1.f, mVolR = 1.f;
	int   mVelocity = 100, mChannelVolume = 127;
	double mBendFactor = 1.0;

	// Moog ladder, "variation 2" from musicdsp.org - the same topology
	// SpaceDub used. Cutoff and resonance are hardcoded in the DXi.
	bool   mLadderEnabled = true;
	double mIn1 = 0.0, mIn2 = 0.0, mIn3 = 0.0, mIn4 = 0.0;
	double mOut1 = 0.0, mOut2 = 0.0, mOut3 = 0.0, mOut4 = 0.0;
};

//------------------------------------------------------------------------
} // namespace ForTran
