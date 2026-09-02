//------------------------------------------------------------------------
// ForTran - the parameter table
//
// Generated from the DXi's CMediaParams::m_aParamInfo. Do not hand-edit a
// range or a default here without recording it in PORTING-NOTES.md - the
// whole point of this table is that the DSP receives what it always did.
//------------------------------------------------------------------------

#include "ForTranParams.h"
#include "Adsr.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace ForTran {

const char* const kFilterStateNames[4]  = { "Off", "Add", "Mult", "Max" };
const char* const kLowPassStateNames[2] = { "Off", "On" };

//------------------------------------------------------------------------
// id                     title                 units  type      plainMin  plainMax  default  intMin  intMax  steps  smoothed
const ParamDef kParams[kNumParams] =
{
	{ kEnable,               "Enabled",             "",   ParamType::Bool,      0.0,    1.0,    1.0,   0.0,   1.0,  1, false },

	{ kHarm1,                "Harmonic 1",          "%",  ParamType::Float,     0.0,  100.0,  100.0,   0.0,   1.0,  0, true  },
	{ kHarm2,                "Harmonic 2",          "%",  ParamType::Float,     0.0,  100.0,   70.0,   0.0,   1.0,  0, true  },
	{ kHarm3,                "Harmonic 3",          "%",  ParamType::Float,     0.0,  100.0,   65.0,   0.0,   1.0,  0, true  },
	{ kHarm4,                "Harmonic 4",          "%",  ParamType::Float,     0.0,  100.0,   60.0,   0.0,   1.0,  0, true  },
	{ kHarm5,                "Harmonic 5",          "%",  ParamType::Float,     0.0,  100.0,   55.0,   0.0,   1.0,  0, true  },
	{ kHarm6,                "Harmonic 6",          "%",  ParamType::Float,     0.0,  100.0,   50.0,   0.0,   1.0,  0, true  },
	{ kHarm7,                "Harmonic 7",          "%",  ParamType::Float,     0.0,  100.0,   45.0,   0.0,   1.0,  0, true  },
	{ kHarm8,                "Harmonic 8",          "%",  ParamType::Float,     0.0,  100.0,   40.0,   0.0,   1.0,  0, true  },
	{ kHarm9,                "Harmonic 9",          "%",  ParamType::Float,     0.0,  100.0,   35.0,   0.0,   1.0,  0, true  },
	{ kHarm10,               "Harmonic 10",         "%",  ParamType::Float,     0.0,  100.0,   30.0,   0.0,   1.0,  0, true  },

	{ kEnvAttack,            "Env Attack",          "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kEnvDecay,             "Env Decay",           "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kEnvSustain,           "Env Sustain",         "%",  ParamType::Float,     0.0,  100.0,   75.0,   0.0,   1.0,  0, true  },

	{ kEnvRelease,           "Env Release",         "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kEnvLevel,             "Env Level",           "%",  ParamType::Float,     0.0,  100.0,  100.0,   0.0,   1.0,  0, true  },

	{ kFormantAttack1,       "Formant Attack1",     "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantDecay1,        "Formant Decay1",      "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantSustain1,      "Formant Sustain1",    "%",  ParamType::Float,     0.0,  100.0,   75.0,   0.0,   1.0,  0, true  },

	{ kFormantRelease1,      "Formant Release1",    "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantLevel1,        "Formant Level1",      "%",  ParamType::Float,     0.0,  100.0,  100.0,   0.0,   1.0,  0, true  },

	{ kFormantAttack2,       "Formant Attack2",     "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantDecay2,        "Formant Decay2",      "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantSustain2,      "Formant Sustain2",    "%",  ParamType::Float,     0.0,  100.0,   75.0,   0.0,   1.0,  0, true  },

	{ kFormantRelease2,      "Formant Release2",    "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantLevel2,        "Formant Level2",      "%",  ParamType::Float,     0.0,  100.0,   60.0,   0.0,   1.0,  0, true  },

	{ kFormantAttack3,       "Formant Attack3",     "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantDecay3,        "Formant Decay3",      "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantSustain3,      "Formant Sustain3",    "%",  ParamType::Float,     0.0,  100.0,   75.0,   0.0,   1.0,  0, true  },

	{ kFormantRelease3,      "Formant Release3",    "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantLevel3,        "Formant Level3",      "%",  ParamType::Float,     0.0,  100.0,   50.0,   0.0,   1.0,  0, true  },

	{ kFormantAttack4,       "Formant Attack4",     "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantDecay4,        "Formant Decay4",      "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantSustain4,      "Formant Sustain4",    "%",  ParamType::Float,     0.0,  100.0,   75.0,   0.0,   1.0,  0, true  },

	{ kFormantRelease4,      "Formant Release4",    "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantLevel4,        "Formant Level4",      "%",  ParamType::Float,     0.0,  100.0,   45.0,   0.0,   1.0,  0, true  },

	{ kFormantAttack5,       "Formant Attack5",     "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantDecay5,        "Formant Decay5",      "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantSustain5,      "Formant Sustain5",    "%",  ParamType::Float,     0.0,  100.0,   75.0,   0.0,   1.0,  0, true  },

	{ kFormantRelease5,      "Formant Release5",    "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantLevel5,        "Formant Level5",      "%",  ParamType::Float,     0.0,  100.0,   40.0,   0.0,   1.0,  0, true  },

	{ kFormantAttack6,       "Formant Attack6",     "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantDecay6,        "Formant Decay6",      "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantSustain6,      "Formant Sustain6",    "%",  ParamType::Float,     0.0,  100.0,   75.0,   0.0,   1.0,  0, true  },

	{ kFormantRelease6,      "Formant Release6",    "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantLevel6,        "Formant Level6",      "%",  ParamType::Float,     0.0,  100.0,   35.0,   0.0,   1.0,  0, true  },

	{ kFormantAttack7,       "Formant Attack7",     "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantDecay7,        "Formant Decay7",      "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantSustain7,      "Formant Sustain7",    "%",  ParamType::Float,     0.0,  100.0,   75.0,   0.0,   1.0,  0, true  },

	{ kFormantRelease7,      "Formant Release7",    "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantLevel7,        "Formant Level7",      "%",  ParamType::Float,     0.0,  100.0,   30.0,   0.0,   1.0,  0, true  },

	{ kFormantAttack8,       "Formant Attack8",     "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantDecay8,        "Formant Decay8",      "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantSustain8,      "Formant Sustain8",    "%",  ParamType::Float,     0.0,  100.0,   75.0,   0.0,   1.0,  0, true  },

	{ kFormantRelease8,      "Formant Release8",    "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantLevel8,        "Formant Level8",      "%",  ParamType::Float,     0.0,  100.0,   25.0,   0.0,   1.0,  0, true  },

	{ kFormantAttack9,       "Formant Attack9",     "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantDecay9,        "Formant Decay9",      "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantSustain9,      "Formant Sustain9",    "%",  ParamType::Float,     0.0,  100.0,   75.0,   0.0,   1.0,  0, true  },

	{ kFormantRelease9,      "Formant Release9",    "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantLevel9,        "Formant Level9",      "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantAttack10,      "Formant Attack10",    "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantDecay10,       "Formant Decay10",     "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantSustain10,     "Formant Sustain10",   "%",  ParamType::Float,     0.0,  100.0,   75.0,   0.0,   1.0,  0, true  },

	{ kFormantRelease10,     "Formant Release10",   "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFormantLevel10,       "Formant Level10",     "%",  ParamType::Float,     0.0,  100.0,   15.0,   0.0,   1.0,  0, true  },

	{ kFormantEnv1,          "Enable Form 1",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantEnv2,          "Enable Form 2",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantEnv3,          "Enable Form 3",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantEnv4,          "Enable Form 4",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantEnv5,          "Enable Form 5",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantEnv6,          "Enable Form 6",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantEnv7,          "Enable Form 7",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantEnv8,          "Enable Form 8",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantEnv9,          "Enable Form 9",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantEnv10,         "Enable Form 10",      "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },

	{ kFormantInv1,          "Invert Form 1",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantInv2,          "Invert Form 2",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantInv3,          "Invert Form 3",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantInv4,          "Invert Form 4",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantInv5,          "Invert Form 5",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantInv6,          "Invert Form 6",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantInv7,          "Invert Form 7",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantInv8,          "Invert Form 8",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantInv9,          "Invert Form 9",       "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kFormantInv10,         "Invert Form 10",      "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },

	{ kTremoloRate,          "Tremolo Rate",        "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kTremoloDepth,         "Tremolo Depth",       "%",  ParamType::Float,     0.0,  100.0,    0.0,   0.0,   1.0,  0, true  },

	{ kVibratoRate,          "Vibrato Rate",        "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kVibratoDepth,         "Vibrato Depth",       "%",  ParamType::Float,     0.0,  100.0,    0.0,   0.0,   1.0,  0, true  },

	{ kCutoff,               "Filter Cutoff",       "",   ParamType::Float,     0.0,  100.0,    0.0,   0.0,   1.0,  0, true  },

	{ kFilterGain,           "Filter Gain",         "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kPoles,                "Filter Poles",        "",   ParamType::Float,     1.0,  100.0,   40.0,   0.0,   1.0,  0, true  },

	{ kFilterAttack,         "Filter Attack",       "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFilterDecay,          "Filter Decay",        "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFilterSustain,        "Filter Sustain",      "%",  ParamType::Float,     0.0,  100.0,   75.0,   0.0,   1.0,  0, true  },

	{ kFilterRelease,        "Filter Release",      "%",  ParamType::Float,     0.0,  100.0,   20.0,   0.0,   1.0,  0, true  },

	{ kFilterLevel,          "Filter Level",        "%",  ParamType::Float,     0.0,  100.0,  100.0,   0.0,   1.0,  0, true  },

	{ kLowPassState,         "Low Pass State",      "",   ParamType::Enum,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },

	{ kHighPassState,        "High Pass State",     "",   ParamType::Enum,      0.0,    3.0,    0.0,   0.0,   3.0,  3, false },

	{ kCombState,            "Comb Pass State",     "",   ParamType::Enum,      0.0,    3.0,    0.0,   0.0,   3.0,  3, false },

	{ kGrassState,           "Grass Pass State",    "",   ParamType::Enum,      0.0,    3.0,    0.0,   0.0,   3.0,  3, false },

	{ kHarmPhase1,           "Phase Harmonic 1",    "%",  ParamType::Float,     0.0,  100.0,   50.0,   0.0,   1.0,  0, true  },
	{ kHarmPhase2,           "Phase Harmonic 2",    "%",  ParamType::Float,     0.0,  100.0,   50.0,   0.0,   1.0,  0, true  },
	{ kHarmPhase3,           "Phase Harmonic 3",    "%",  ParamType::Float,     0.0,  100.0,   50.0,   0.0,   1.0,  0, true  },
	{ kHarmPhase4,           "Phase Harmonic 4",    "%",  ParamType::Float,     0.0,  100.0,   50.0,   0.0,   1.0,  0, true  },
	{ kHarmPhase5,           "Phase Harmonic 5",    "%",  ParamType::Float,     0.0,  100.0,   50.0,   0.0,   1.0,  0, true  },
	{ kHarmPhase6,           "Phase Harmonic 6",    "%",  ParamType::Float,     0.0,  100.0,   50.0,   0.0,   1.0,  0, true  },
	{ kHarmPhase7,           "Phase Harmonic 7",    "%",  ParamType::Float,     0.0,  100.0,   50.0,   0.0,   1.0,  0, true  },
	{ kHarmPhase8,           "Phase Harmonic 8",    "%",  ParamType::Float,     0.0,  100.0,   50.0,   0.0,   1.0,  0, true  },
	{ kHarmPhase9,           "Phase Harmonic 9",    "%",  ParamType::Float,     0.0,  100.0,   50.0,   0.0,   1.0,  0, true  },
	{ kHarmPhase10,          "Phase Harmonic 10",   "%",  ParamType::Float,     0.0,  100.0,   50.0,   0.0,   1.0,  0, true  },

	{ kHarmInv1,             "Invert Harmonic 1",   "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kHarmInv2,             "Invert Harmonic 2",   "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kHarmInv3,             "Invert Harmonic 3",   "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kHarmInv4,             "Invert Harmonic 4",   "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kHarmInv5,             "Invert Harmonic 5",   "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kHarmInv6,             "Invert Harmonic 6",   "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kHarmInv7,             "Invert Harmonic 7",   "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kHarmInv8,             "Invert Harmonic 8",   "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kHarmInv9,             "Invert Harmonic 9",   "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },
	{ kHarmInv10,            "Invert Harmonic 10",  "",   ParamType::Bool,      0.0,    1.0,    0.0,   0.0,   1.0,  1, false },

	// -- the per-harmonic LFO, not from the DXi --------------------------
	//
	// Rate reads in Hz and is logarithmic over 0.05 .. 20 Hz - see
	// harmonicLfoHz() in Voice.h, which is the one definition the DSP and
	// the panel both use. The defaults are the DXi's own intended seeding,
	// dFormantsLFOStep[i-1] = 2*pi / (i * sampleRate): harmonic n wobbles
	// at 1/n Hz, which puts harmonic 1 dead centre on the knob.
	//
	// DEPTH DEFAULTS TO ZERO, all ten of them. The LFO is off until it is
	// turned on, so adding it changes no existing sound and costs nothing -
	// Voice skips the term entirely at depth zero. The DXi's abandoned
	// dFormantLFODepth[i-1] = 3.0/i is NOT carried: it was never read, and
	// a depth above 1 inverts the partial twice a cycle.
	{ kHarmLfoRate1,          "LFO Rate Harm 1",     "%",  ParamType::Float,     0.0,  100.0,  50.00,   0.0,   1.0,  0, true  },
	{ kHarmLfoRate2,          "LFO Rate Harm 2",     "%",  ParamType::Float,     0.0,  100.0,  38.43,   0.0,   1.0,  0, true  },
	{ kHarmLfoRate3,          "LFO Rate Harm 3",     "%",  ParamType::Float,     0.0,  100.0,  31.66,   0.0,   1.0,  0, true  },
	{ kHarmLfoRate4,          "LFO Rate Harm 4",     "%",  ParamType::Float,     0.0,  100.0,  26.86,   0.0,   1.0,  0, true  },
	{ kHarmLfoRate5,          "LFO Rate Harm 5",     "%",  ParamType::Float,     0.0,  100.0,  23.14,   0.0,   1.0,  0, true  },
	{ kHarmLfoRate6,          "LFO Rate Harm 6",     "%",  ParamType::Float,     0.0,  100.0,  20.09,   0.0,   1.0,  0, true  },
	{ kHarmLfoRate7,          "LFO Rate Harm 7",     "%",  ParamType::Float,     0.0,  100.0,  17.52,   0.0,   1.0,  0, true  },
	{ kHarmLfoRate8,          "LFO Rate Harm 8",     "%",  ParamType::Float,     0.0,  100.0,  15.29,   0.0,   1.0,  0, true  },
	{ kHarmLfoRate9,          "LFO Rate Harm 9",     "%",  ParamType::Float,     0.0,  100.0,  13.33,   0.0,   1.0,  0, true  },
	{ kHarmLfoRate10,         "LFO Rate Harm 10",    "%",  ParamType::Float,     0.0,  100.0,  11.57,   0.0,   1.0,  0, true  },

	{ kHarmLfoDepth1,         "LFO Depth Harm 1",    "%",  ParamType::Float,     0.0,  100.0,   0.00,   0.0,   1.0,  0, true  },
	{ kHarmLfoDepth2,         "LFO Depth Harm 2",    "%",  ParamType::Float,     0.0,  100.0,   0.00,   0.0,   1.0,  0, true  },
	{ kHarmLfoDepth3,         "LFO Depth Harm 3",    "%",  ParamType::Float,     0.0,  100.0,   0.00,   0.0,   1.0,  0, true  },
	{ kHarmLfoDepth4,         "LFO Depth Harm 4",    "%",  ParamType::Float,     0.0,  100.0,   0.00,   0.0,   1.0,  0, true  },
	{ kHarmLfoDepth5,         "LFO Depth Harm 5",    "%",  ParamType::Float,     0.0,  100.0,   0.00,   0.0,   1.0,  0, true  },
	{ kHarmLfoDepth6,         "LFO Depth Harm 6",    "%",  ParamType::Float,     0.0,  100.0,   0.00,   0.0,   1.0,  0, true  },
	{ kHarmLfoDepth7,         "LFO Depth Harm 7",    "%",  ParamType::Float,     0.0,  100.0,   0.00,   0.0,   1.0,  0, true  },
	{ kHarmLfoDepth8,         "LFO Depth Harm 8",    "%",  ParamType::Float,     0.0,  100.0,   0.00,   0.0,   1.0,  0, true  },
	{ kHarmLfoDepth9,         "LFO Depth Harm 9",    "%",  ParamType::Float,     0.0,  100.0,   0.00,   0.0,   1.0,  0, true  },
	{ kHarmLfoDepth10,        "LFO Depth Harm 10",   "%",  ParamType::Float,     0.0,  100.0,   0.00,   0.0,   1.0,  0, true  },

	// The default is the knob's centre, -20 dB. See the note in the header:
	// the DXi's own staging is the TOP of this control, not the default.
	{ kOutputLevel,           "Output",              "%",  ParamType::Float,     0.0,  100.0,  50.00,   0.0,   1.0,  0, true  },

	// Defaults ON, which is the DXi: the ladder is in the path unless it is
	// deliberately taken out.
	{ kLadderEnable,          "Ladder Filter",       "",   ParamType::Bool,      0.0,    1.0,    1.0,   0.0,   1.0,  1, false },
};

//------------------------------------------------------------------------
// The formant block is ten repeats of attack, decay, sustain, release,
// level - so the role of a formant parameter is its offset modulo 5. The
// enum is asserted to still look like that, because it is generated.
//------------------------------------------------------------------------
namespace {
constexpr int kFormantStride = 5;
static_assert (kFormantDecay1   == kFormantAttack1 + 1, "formant block reordered");
static_assert (kFormantSustain1 == kFormantAttack1 + 2, "formant block reordered");
static_assert (kFormantRelease1 == kFormantAttack1 + 3, "formant block reordered");
static_assert (kFormantLevel1   == kFormantAttack1 + 4, "formant block reordered");
static_assert (kFormantAttack2  == kFormantAttack1 + kFormantStride, "formant stride changed");
static_assert (kFormantLevel10  == kFormantAttack1 + 10 * kFormantStride - 1,
               "the formant block is not ten times five parameters");
} // anonymous namespace

TimeRole envelopeTimeRole (Steinberg::Vst::ParamID id)
{
	switch (id)
	{
		case kEnvAttack:  case kFilterAttack:  return TimeRole::Attack;
		case kEnvDecay:   case kFilterDecay:   return TimeRole::Decay;
		case kEnvRelease: case kFilterRelease: return TimeRole::Release;
		default: break;
	}

	if (id >= kFormantAttack1 && id <= kFormantLevel10)
	{
		switch ((id - kFormantAttack1) % kFormantStride)
		{
			case 0: return TimeRole::Attack;
			case 1: return TimeRole::Decay;
			case 3: return TimeRole::Release;
			default: return TimeRole::None;   // sustain and level are levels
		}
	}

	return TimeRole::None;
}

//------------------------------------------------------------------------
double envelopeTimeSeconds (Steinberg::Vst::ParamID id, double internal)
{
	const TimeRole role = envelopeTimeRole (id);
	if (role == TimeRole::None)
		return -1.0;

	const double max = (role == TimeRole::Release) ? Adsr::kMaxReleaseSeconds
	                                               : Adsr::kMaxAttackDecaySeconds;
	const double r = (internal > 0.0) ? ((internal < 1.0) ? internal : 1.0) : 0.0;
	const double seconds = r * r * max;

	// Adsr floors every segment at one millisecond, so the bottom of the
	// travel is 1 ms and not zero. Say so rather than showing "0 ms".
	return (seconds > 0.001) ? seconds : 0.001;
}

//------------------------------------------------------------------------
double envelopeTimeToInternal (Steinberg::Vst::ParamID id, double seconds)
{
	const TimeRole role = envelopeTimeRole (id);
	if (role == TimeRole::None)
		return -1.0;

	const double max = (role == TimeRole::Release) ? Adsr::kMaxReleaseSeconds
	                                               : Adsr::kMaxAttackDecaySeconds;
	// Adsr floors every segment at a millisecond, so every setting from 0 up
	// to sqrt(0.001/max) produces exactly 1 ms. Map that whole band back to
	// 0 - the canonical member - so "1 ms" round-trips to the bottom of the
	// travel instead of landing part way up it.
	if (!(seconds > 0.001))
		return 0.0;
	const double r = std::sqrt (seconds / max);
	return (r < 1.0) ? r : 1.0;
}

//------------------------------------------------------------------------
void formatEnvelopeTime (double seconds, char* out, std::size_t outSize)
{
	if (!out || outSize == 0)
		return;

	// The threshold is 0.9995 rather than 1.0 so nothing ever prints as
	// "1000 ms" - it rounds up into "1.00 s" instead.
	if (seconds < 0.9995)
		std::snprintf (out, outSize, "%.0f ms", seconds * 1000.0);
	else
		std::snprintf (out, outSize, "%.2f s", seconds);
}

//------------------------------------------------------------------------
bool parseEnvelopeTime (const char* text, double& seconds)
{
	if (!text)
		return false;

	char* end = nullptr;
	const double value = std::strtod (text, &end);
	if (end == text)
		return false;               // nothing numeric at the front

	while (*end == ' ' || *end == '\t')
		++end;

	// Case-insensitive, and only the leading letters matter: "ms", "mS",
	// "msec", "millisecond" all mean the same thing.
	const bool milli = (end[0] == 'm' || end[0] == 'M') &&
	                   (end[1] == 's' || end[1] == 'S');

	// A bare number is taken as seconds. It is the parameter's natural unit,
	// and the alternative - guessing from the magnitude - would make "1"
	// mean different things at different points in the control's travel.
	seconds = milli ? value / 1000.0 : value;
	return true;
}

//------------------------------------------------------------------------
bool isPhaseParam (Steinberg::Vst::ParamID id)
{
	return id >= kHarmPhase1 && id <= kHarmPhase10;
}

//------------------------------------------------------------------------
double phaseDegrees (double internal)
{
	const double r = (internal > 0.0) ? ((internal < 1.0) ? internal : 1.0) : 0.0;
	return -180.0 * (0.5 - r);
}

//------------------------------------------------------------------------
double phaseToInternal (double degrees)
{
	const double r = 0.5 + degrees / 180.0;
	return (r > 0.0) ? ((r < 1.0) ? r : 1.0) : 0.0;
}

//------------------------------------------------------------------------
void formatPhase (double degrees, char* out, std::size_t outSize)
{
	if (!out || outSize == 0)
		return;
	// Round before testing the sign, so a value that rounds to zero does not
	// come out as "-0".
	const double rounded = (degrees < 0.0) ? -std::floor (-degrees + 0.5)
	                                       :  std::floor (degrees + 0.5);
	if (rounded == 0.0)
		std::snprintf (out, outSize, "0");
	else
		std::snprintf (out, outSize, "%+.0f", rounded);
}

//------------------------------------------------------------------------
double outputGain (double control)
{
	if (!(control > 0.0))
		return 0.0;                            // the bottom of the knob is off
	const double c = (control < 1.0) ? control : 1.0;
	return std::pow (10.0, (c - 1.0) * kOutputRangeDb / 20.0);
}

//------------------------------------------------------------------------
double outputDecibels (double control)
{
	if (!(control > 0.0))
		return -1e9;
	const double c = (control < 1.0) ? control : 1.0;
	return (c - 1.0) * kOutputRangeDb;
}

//------------------------------------------------------------------------
double outputControlFromDb (double db)
{
	const double c = 1.0 + db / kOutputRangeDb;
	return (c > 0.0) ? ((c < 1.0) ? c : 1.0) : 0.0;
}

//------------------------------------------------------------------------
} // namespace ForTran
