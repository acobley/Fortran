//------------------------------------------------------------------------
// ForTran - parameter definitions
//
// Generated from the DXi's own Parameters.h (both the enum and the
// CMediaParams::m_aParamInfo table), so the order, the ranges and the
// defaults are the original's by construction rather than by transcription.
//
// As in the SpaceDub port, every parameter carries three ranges:
//   * VST3 normalised, 0..1, which is what the host works in;
//   * "plain", the DXi *external* range, so the numbers on screen match;
//   * "internal", what the DSP was actually handed, via toInternal(),
//     reproducing ParamInfo::MapToInternal.
//
// Deviations from the DXi table are marked DEVIATION and explained in
// PORTING-NOTES.md section 2. There are two.
//------------------------------------------------------------------------

#pragma once

#include <cstddef>

#include "pluginterfaces/vst/vsttypes.h"

namespace ForTran {

//------------------------------------------------------------------------
enum Param : Steinberg::Vst::ParamID
{
	kEnable,               // PARAM_ENABLE

	kHarm1,                // PARAM_HARM1
	kHarm2,                // PARAM_HARM2
	kHarm3,                // PARAM_HARM3
	kHarm4,                // PARAM_HARM4
	kHarm5,                // PARAM_HARM5
	kHarm6,                // PARAM_HARM6
	kHarm7,                // PARAM_HARM7
	kHarm8,                // PARAM_HARM8
	kHarm9,                // PARAM_HARM9
	kHarm10,               // PARAM_HARM10

	kEnvAttack,            // PARAM_ENVATTACK

	kEnvDecay,             // PARAM_ENVDECAY

	kEnvSustain,           // PARAM_ENVSUSTAIN

	kEnvRelease,           // PARAM_ENVRELEASE

	kEnvLevel,             // PARAM_ENVLEVEL

	kFormantAttack1,       // PARAM_FORMANTATTACK1

	kFormantDecay1,        // PARAM_FORMANTDECAY1

	kFormantSustain1,      // PARAM_FORMANTSUSTAIN1

	kFormantRelease1,      // PARAM_FORMANTRELEASE1

	kFormantLevel1,        // PARAM_FORMANTLEVEL1

	kFormantAttack2,       // PARAM_FORMANTATTACK2

	kFormantDecay2,        // PARAM_FORMANTDECAY2

	kFormantSustain2,      // PARAM_FORMANTSUSTAIN2

	kFormantRelease2,      // PARAM_FORMANTRELEASE2

	kFormantLevel2,        // PARAM_FORMANTLEVEL2

	kFormantAttack3,       // PARAM_FORMANTATTACK3

	kFormantDecay3,        // PARAM_FORMANTDECAY3

	kFormantSustain3,      // PARAM_FORMANTSUSTAIN3

	kFormantRelease3,      // PARAM_FORMANTRELEASE3

	kFormantLevel3,        // PARAM_FORMANTLEVEL3

	kFormantAttack4,       // PARAM_FORMANTATTACK4

	kFormantDecay4,        // PARAM_FORMANTDECAY4

	kFormantSustain4,      // PARAM_FORMANTSUSTAIN4

	kFormantRelease4,      // PARAM_FORMANTRELEASE4

	kFormantLevel4,        // PARAM_FORMANTLEVEL4

	kFormantAttack5,       // PARAM_FORMANTATTACK5

	kFormantDecay5,        // PARAM_FORMANTDECAY5

	kFormantSustain5,      // PARAM_FORMANTSUSTAIN5

	kFormantRelease5,      // PARAM_FORMANTRELEASE5

	kFormantLevel5,        // PARAM_FORMANTLEVEL5

	kFormantAttack6,       // PARAM_FORMANTATTACK6

	kFormantDecay6,        // PARAM_FORMANTDECAY6

	kFormantSustain6,      // PARAM_FORMANTSUSTAIN6

	kFormantRelease6,      // PARAM_FORMANTRELEASE6

	kFormantLevel6,        // PARAM_FORMANTLEVEL6

	kFormantAttack7,       // PARAM_FORMANTATTACK7

	kFormantDecay7,        // PARAM_FORMANTDECAY7

	kFormantSustain7,      // PARAM_FORMANTSUSTAIN7

	kFormantRelease7,      // PARAM_FORMANTRELEASE7

	kFormantLevel7,        // PARAM_FORMANTLEVEL7

	kFormantAttack8,       // PARAM_FORMANTATTACK8

	kFormantDecay8,        // PARAM_FORMANTDECAY8

	kFormantSustain8,      // PARAM_FORMANTSUSTAIN8

	kFormantRelease8,      // PARAM_FORMANTRELEASE8

	kFormantLevel8,        // PARAM_FORMANTLEVEL8

	kFormantAttack9,       // PARAM_FORMANTATTACK9

	kFormantDecay9,        // PARAM_FORMANTDECAY9

	kFormantSustain9,      // PARAM_FORMANTSUSTAIN9

	kFormantRelease9,      // PARAM_FORMANTRELEASE9

	kFormantLevel9,        // PARAM_FORMANTLEVEL9

	kFormantAttack10,      // PARAM_FORMANTATTACK10

	kFormantDecay10,       // PARAM_FORMANTDECAY10

	kFormantSustain10,     // PARAM_FORMANTSUSTAIN10

	kFormantRelease10,     // PARAM_FORMANTRELEASE10

	kFormantLevel10,       // PARAM_FORMANTLEVEL10

	kFormantEnv1,          // PARAM_FORMANTENV1
	kFormantEnv2,          // PARAM_FORMANTENV2
	kFormantEnv3,          // PARAM_FORMANTENV3
	kFormantEnv4,          // PARAM_FORMANTENV4
	kFormantEnv5,          // PARAM_FORMANTENV5
	kFormantEnv6,          // PARAM_FORMANTENV6
	kFormantEnv7,          // PARAM_FORMANTENV7
	kFormantEnv8,          // PARAM_FORMANTENV8
	kFormantEnv9,          // PARAM_FORMANTENV9
	kFormantEnv10,         // PARAM_FORMANTENV10

	kFormantInv1,          // PARAM_FORMANTINV1
	kFormantInv2,          // PARAM_FORMANTINV2
	kFormantInv3,          // PARAM_FORMANTINV3
	kFormantInv4,          // PARAM_FORMANTINV4
	kFormantInv5,          // PARAM_FORMANTINV5
	kFormantInv6,          // PARAM_FORMANTINV6
	kFormantInv7,          // PARAM_FORMANTINV7
	kFormantInv8,          // PARAM_FORMANTINV8
	kFormantInv9,          // PARAM_FORMANTINV9
	kFormantInv10,         // PARAM_FORMANTINV10

	kTremoloRate,          // PARAM_TREMELORATE

	kTremoloDepth,         // PARAM_TREMELODEPTH

	kVibratoRate,          // PARAM_VIBRATORATE

	kVibratoDepth,         // PARAM_VIBRATODEPTH

	kCutoff,               // PARAM_CUTOFF

	kFilterGain,           // PARAM_FILTERGAIN

	kPoles,                // PARAM_POLES

	kFilterAttack,         // PARAM_FILTERATTACK

	kFilterDecay,          // PARAM_FILTERDECAY

	kFilterSustain,        // PARAM_FILTERSUSTAIN

	kFilterRelease,        // PARAM_FILTERRELEASE

	kFilterLevel,          // PARAM_FILTERLEVEL

	kLowPassState,         // PARAM_LPSTATE

	kHighPassState,        // PARAM_HPSTATE

	kCombState,            // PARAM_COMBSTATE

	kGrassState,           // PARAM_GRASSSTATE

	kHarmPhase1,           // PARAM_HARMPHASE1
	kHarmPhase2,           // PARAM_HARMPHASE2
	kHarmPhase3,           // PARAM_HARMPHASE3
	kHarmPhase4,           // PARAM_HARMPHASE4
	kHarmPhase5,           // PARAM_HARMPHASE5
	kHarmPhase6,           // PARAM_HARMPHASE6
	kHarmPhase7,           // PARAM_HARMPHASE7
	kHarmPhase8,           // PARAM_HARMPHASE8
	kHarmPhase9,           // PARAM_HARMPHASE9
	kHarmPhase10,          // PARAM_HARMPHASE10

	kHarmInv1,             // PARAM_HARMINV1
	kHarmInv2,             // PARAM_HARMINV2
	kHarmInv3,             // PARAM_HARMINV3
	kHarmInv4,             // PARAM_HARMINV4
	kHarmInv5,             // PARAM_HARMINV5
	kHarmInv6,             // PARAM_HARMINV6
	kHarmInv7,             // PARAM_HARMINV7
	kHarmInv8,             // PARAM_HARMINV8
	kHarmInv9,             // PARAM_HARMINV9
	kHarmInv10,            // PARAM_HARMINV10

	// -- NOT FROM THE DXi -------------------------------------------------
	//
	// The per-harmonic amplitude LFO the DXi started and never finished: it
	// allocated dFormantsLFOTheta/Step/Depth[10] per note, threaded a
	// dFormantLFO multiplier through all ten harmonic terms, and then never
	// wrote to any of it. See PORTING-NOTES section 4.
	//
	// These twenty are appended AFTER every DXi parameter so the existing
	// ids do not move; a project saved before they existed still loads.
	kHarmLfoRate1,
	kHarmLfoRate2,
	kHarmLfoRate3,
	kHarmLfoRate4,
	kHarmLfoRate5,
	kHarmLfoRate6,
	kHarmLfoRate7,
	kHarmLfoRate8,
	kHarmLfoRate9,
	kHarmLfoRate10,

	kHarmLfoDepth1,
	kHarmLfoDepth2,
	kHarmLfoDepth3,
	kHarmLfoDepth4,
	kHarmLfoDepth5,
	kHarmLfoDepth6,
	kHarmLfoDepth7,
	kHarmLfoDepth8,
	kHarmLfoDepth9,
	kHarmLfoDepth10,

	/** Output trim. Also not from the DXi, and appended after everything so
	    the ids before it never move. See the note below on why it exists. */
	kOutputLevel,

	/** Switches the output ladder in and out. Not from the DXi either - it
	    is there because the ladder turns out to be doing very little for
	    most of the keyboard, and the only way to judge that is to hear it
	    both ways. See the note below. */
	kLadderEnable,

	/** Pan. The DXi had no pan control at all - only CC10 - so this is new,
	    and it OFFSETS the controller rather than replacing it: at its
	    centre CC10 comes through untouched. See the note below. */
	kPan,

	/** Portamento. New: the DXi had no glide of any kind, on any note.
	    Zero is off, and zero is the default, so no existing patch and no
	    saved project starts gliding. See the note below. */
	kPortamento,

	kNumParams,

	kBypass = 1000,         // standard VST3 bypass (kIsBypass)

	/** MIDI controller destinations, published through IMidiMapping so the
	    host routes CC7, CC10 and the pitch wheel into them. They are not
	    part of the DXi's parameter set and are NOT saved in the state -
	    they are live controller values, not settings. */
	kMidiVolume    = 1001,  // CC7
	kMidiPan       = 1002,  // CC10
	kMidiPitchBend = 1003
};

//------------------------------------------------------------------------
/** The DXi had four scalar types; SpaceDub only needed three. */
enum class ParamType
{
	Float,
	Bool,
	Enum,
	Int
};

//------------------------------------------------------------------------
struct ParamDef
{
	Steinberg::Vst::ParamID id;
	const char* title;
	const char* units;
	ParamType   type;
	double      plainMin;       // DXi "external" range
	double      plainMax;
	double      plainDefault;
	double      internalMin;    // range the DSP expects
	double      internalMax;
	int         stepCount;      // 0 = continuous
	bool        smoothed;

	//--------------------------------------------------------------------
	double toPlain (double normalized) const
	{
		return plainMin + normalized * (plainMax - plainMin);
	}

	double toNormalized (double plain) const
	{
		if (plainMax == plainMin)
			return 0.0;
		return (plain - plainMin) / (plainMax - plainMin);
	}

	/** Reproduces ParamInfo::MapToInternal from the DXi. */
	double toInternal (double normalized) const
	{
		if (type == ParamType::Bool)
			return (normalized < 0.5) ? 0.0 : 1.0;

		const double v = internalMin + normalized * (internalMax - internalMin);
		if (type == ParamType::Enum || type == ParamType::Int)
			return static_cast<double> (static_cast<long> (v + 0.5));
		return v;
	}

	double defaultNormalized () const { return toNormalized (plainDefault); }
};

//------------------------------------------------------------------------
extern const ParamDef kParams[kNumParams];

/** Look a definition up by id. Ids are dense, so this is just an index -
    but kBypass is 1000, so RANGE-CHECK before calling this. */
inline const ParamDef& paramDef (Steinberg::Vst::ParamID id) { return kParams[id]; }

/** High pass, comb and grass take these four values, in this order - their
    NewSpin controls were built with SetRange(0,3) and these AddValue()
    strings (ForTranPropPage.cpp:357-377).

    LOW PASS IS DIFFERENT: SetRange(0,1), Off/On only (line 352). It is the
    one filter-state parameter whose range in the DXi's Parameters.h was
    already correct. See PORTING-NOTES section 2. */
extern const char* const kFilterStateNames[4];
extern const char* const kLowPassStateNames[2];

//------------------------------------------------------------------------
// Envelope time parameters
//
// Twelve envelopes on a voice - amplitude, filter and ten formants - each
// with an attack, a decay and a release. The parameter table calls all
// three a percentage, because the DXi's did, but they are times: `Adsr`
// maps the control as `rate^2 * maxSeconds`, 5 s for attack and decay and
// 10 s for release (see the TIME RANGE note in Adsr.h). The editor shows
// the time rather than the percentage, so the mapping is written down here,
// once, beside the table it belongs to.
//------------------------------------------------------------------------
enum class TimeRole { None, Attack, Decay, Release };

/** Which envelope segment, if any, this parameter sets the length of.
    Sustain and level controls are levels, not times, and return None. */
TimeRole envelopeTimeRole (Steinberg::Vst::ParamID id);

/** The segment length in seconds for a control at `internal` (0..1), or a
    negative number if the parameter is not a time. Matches what `Adsr`
    computes, including its one-millisecond floor, so the readout cannot
    disagree with the envelope. */
double envelopeTimeSeconds (Steinberg::Vst::ParamID id, double internal);

/** The inverse: the control setting (0..1) that gives `seconds`, clamped to
    the control's travel. Not a time parameter -> a negative number. */
double envelopeTimeToInternal (Steinberg::Vst::ParamID id, double seconds);

/** Render a time the one way it is rendered everywhere: "200 ms" below a
    second, "1.25 s" at or above one. The editor's readout and the host's
    parameter display both go through this, so the two cannot drift apart.
    `outSize` should be at least 16. */
void formatEnvelopeTime (double seconds, char* out, std::size_t outSize);

/** Read a time back. Accepts "200 ms", "1.25 s", "1.25 sec" and a bare
    number, which is taken as SECONDS. Returns false if there is no number
    at the front of the string. */
bool parseEnvelopeTime (const char* text, double& seconds);

//------------------------------------------------------------------------
// Pan
//
// New. The DXi read CC10 in synthNote's prologue (ForTran.cpp:773) and had
// no control for it anywhere - no parameter, no dialog control, no resource
// id - so the only way to place a voice was to send the controller.
//
// This OFFSETS CC10 rather than replacing it: the knob's centre leaves the
// controller exactly as it arrives, and the two are summed in MIDI units
// and clamped. Automating either still works, and a host that sends no CC10
// simply starts from centre.
//------------------------------------------------------------------------

/** Combine the incoming CC10 (0..127, 64 centred) with the knob (0..1, 0.5
    centred) into the pan the voice should use. */
int combinePan (int cc10, double knob);

/** "centre", "L 42", "R 42" - the knob as an offset, not a position, since
    that is what it is. `outSize` should be at least 12. */
void formatPan (double knob, char* out, std::size_t outSize);

//------------------------------------------------------------------------
// Harmonic phase
//
// The ten "Phase Harmonic n" controls are a real per-partial phase shift
// and always were - the DXi applies `dPIm * (0.5 - control)` with
// `dPIm = -pi` (ForTran.cpp:791), which is
//
//     control 0 -> -90 degrees,  0.5 -> 0,  1 -> +90 degrees
//
// exactly half a turn end to end, 45 degrees per quarter. The table calls
// them percentages because the DXi's did; a percentage tells you nothing
// useful about a phase, so they read in degrees instead.
//
// The separate Invert buttons are a further 180 degree flip, applied to the
// level rather than the phase. Between them the two controls reach the whole
// circle.
//------------------------------------------------------------------------

//------------------------------------------------------------------------
// Output trim
//
// NOT from the DXi, and it is here because the DXi's gain staging cancels
// itself out. The note gain carries `* 0.25` with the original's own comment
// "allow for 12dB of headroom" - and then the ladder ends `fSamp = m_out4 *
// 5.0`, four lines later, which throws that headroom away and then some:
// 0.25 * 5 = 1.25, a net gain ABOVE unity.
//
// The factory patch peaks at 3.3 on ONE note and 19.3 on sixteen. Nothing
// clips inside the plug-in, so it costs nothing but headroom - except that
// it also destroys velocity, because every velocity from 16 to 127 lands
// above the host's ceiling and comes out at exactly the same level.
//
// The trim is a plain gain on the mixed output: 0..100 % maps to silence
// and then -40 dB up to 0 dB, so the TOP of the knob is the DXi's own
// staging and nothing is lost. The default is the knob's centre, -20 dB,
// which puts a single factory note at about -10 dBFS.
//------------------------------------------------------------------------
constexpr double kOutputRangeDb = 40.0;

/** Trim control (0..1) -> a linear gain. Zero is silence. */
double outputGain (double control);

/** The same in decibels, for display. Returns -1e9 at silence. */
double outputDecibels (double control);

/** The inverse of outputDecibels, clamped to the knob's travel. */
double outputControlFromDb (double db);

//------------------------------------------------------------------------
// Portamento
//
// New, and the DXi has nothing to say about it: it had no glide anywhere,
// and a DXi never owned its voices in any case (PORTING-NOTES section 7),
// so there was no note-to-note state to glide with.
//
// TWO DECISIONS, both of them audible:
//
// * CONSTANT TIME, not constant rate. Every glide takes the same time
//   whatever the interval, so the notes of a chord change arrive together.
//   A constant rate - a fixed number of semitones per second - makes wide
//   leaps take proportionally longer, which is more like a real glissando
//   and less like something you can play chords with.
//
// * EXPONENTIAL IN PITCH, i.e. linear in semitones. The glide multiplies
//   the phase step by a constant ratio each sample rather than adding a
//   constant number of hertz, so an octave downwards sweeps as evenly as
//   an octave upwards. Linear-in-hertz glide spends most of its time at
//   the top and arrives with a lurch.
//
// The control is squared, as Adsr squares its times, so the short glides
// that are actually musical get most of the travel: the knob's centre is
// a quarter of the maximum, not half of it.
//------------------------------------------------------------------------
constexpr double kPortamentoMaxSeconds = 2.0;

/** Glide control (0..1) -> seconds. Exactly zero is off. */
double portamentoSeconds (double control);

/** The inverse, clamped to the knob's travel. */
double portamentoControlFromSeconds (double seconds);

//------------------------------------------------------------------------
/** True for the ten per-harmonic phase controls. */
bool isPhaseParam (Steinberg::Vst::ParamID id);

/** Degrees for a control at `internal` (0..1): -90 .. +90. */
double phaseDegrees (double internal);

/** The inverse, clamped to the control's travel. */
double phaseToInternal (double degrees);

/** The number alone - "+45", "0", "-90" - with NO degree sign. The sign is
    not ASCII, and the two places this is shown encode text differently: the
    editor's readout is UTF-8 and the host's is UTF-16. Each appends its own.
    `outSize` should be at least 8. */
void formatPhase (double degrees, char* out, std::size_t outSize);

//------------------------------------------------------------------------
} // namespace ForTran
