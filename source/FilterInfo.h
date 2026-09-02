//------------------------------------------------------------------------
// ForTran - filter response tables
//
// Port of the DXi's FilterInfo class. No SDK types, so it builds and runs
// standalone - see tests/FilterInfoTests.cpp.
//
// This is not a time-domain filter. ForTran is additive, and the "filter"
// is a magnitude and phase looked up per partial: 552 frequency bins on a
// 1/48-octave grid (a quarter of a semitone), anchored so bin 276 is
// 440 Hz. Four shapes are computed over that grid - low pass, high pass,
// comb and "grass" (the comb's complement) - and each of a note's ten
// harmonics reads the shape at its own bin.
//
// See PORTING-NOTES section 6 for what changed and why.
//------------------------------------------------------------------------

#pragma once

namespace ForTran {

//------------------------------------------------------------------------
class FilterInfo
{
public:
	static constexpr int kNumBins  = 552;
	static constexpr int kNumParts = 10;   // harmonics sampled per note

	/** The DXi's four shapes. */
	enum class Shape { LowPass, HighPass, Comb, Grass };

	FilterInfo ();

	// -- controls; all take the plug-in's *internal* ranges ---------------
	/** DXi: SetFeedback (PARAM_FILTERGAIN * 3.9). */
	void setFeedback (float feedback);
	/** DXi: SetPoles (1 + PARAM_POLES * 10), truncated to an int. */
	void setPoles (float poles);
	/** DXi: SetCutoff (PARAM_CUTOFF), 0..1 across the whole bin range. */
	void setCutoff (float cutoff01);

	/** True when a control has moved since the last calculate(). */
	bool needsRecalculation () const { return mDirty; }

	/** Rebuild all four shapes. 552 bins of pow/atan/sin per shape, so this
	    is called once per block when a control has moved - never per sample,
	    and never per note. */
	void calculate ();

	/** The ten harmonic magnitudes and phases for a note, where `bin` is the
	    filter's own index (the DXi's `Note`), not a MIDI note number. */
	void values (Shape shape, int bin, float* magnitude, float* theta) const;

	/** How each shape folds into the running result.
	    Low pass is Off/On only; the other three take all four. */
	enum class Mode { Off = 0, Add = 1, Mult = 2, Max = 3 };

	struct ShapeStates
	{
		int lowPass  = 0;   // 0 or 1
		int highPass = 0;   // 0..3
		int comb     = 0;
		int grass    = 0;
	};

	/** Fold the four shapes into one set of ten magnitudes and phases, in the
	    DXi's order: low pass, high pass, comb, grass.

	    Three deliberate corrections against the original are applied here;
	    all three are set out in PORTING-NOTES section 6. In short: Max works
	    (it wrote into a scratch array and never back), the "has anything
	    contributed yet" test is a flag rather than a list of named
	    predecessors, and Mult and Max leave the phase alone. */
	void combine (const ShapeStates& states, int bin, float* magnitude, float* theta) const;

	/** The whole magnitude curve for one shape. */
	const float* magnitudeCurve (Shape shape) const;

	/** The four shapes combined across every bin - what the editor's
	    response panel draws. Uses the same rules as combine(), so the
	    picture and the sound cannot disagree; the DXi had a second,
	    separately-buggy copy of this in its property page.
	    `out` must hold kNumBins floats. */
	void combinedCurve (const ShapeStates& states, float* out) const;

	/** Bin -> Hz, for the editor. */
	float binFrequency (int bin) const;

private:
	float mFrequency[kNumBins] = {};

	float mLowPassMagnitude[kNumBins]  = {};
	float mLowPassTheta[kNumBins]      = {};
	float mHighPassMagnitude[kNumBins] = {};
	float mHighPassTheta[kNumBins]     = {};
	float mCombMagnitude[kNumBins]     = {};
	float mCombTheta[kNumBins]         = {};
	float mGrassMagnitude[kNumBins]    = {};
	float mGrassTheta[kNumBins]        = {};

	float mCutoff   = 440.f;
	float mFeedback = 0.5f;
	float mPoles    = 4.f;
	bool  mDirty    = false;
};

//------------------------------------------------------------------------
} // namespace ForTran
