//------------------------------------------------------------------------
// ForTran - parameters -> VoiceParams
//------------------------------------------------------------------------

#include "VoiceParamsMap.h"

namespace ForTran {

namespace {
inline double internalOf (const double* normalized, Param id)
{
	return kParams[id].toInternal (normalized[id]);
}
} // anonymous namespace

//------------------------------------------------------------------------
void buildVoiceParams (const double* normalized, VoiceParams& out)
{
	auto v = [normalized] (Param id) { return static_cast<float> (internalOf (normalized, id)); };
	auto b = [normalized] (Param id) { return internalOf (normalized, id) >= 0.5; };

	for (int i = 0; i < 10; ++i)
	{
		out.harmLevel[i]  = v (static_cast<Param> (kHarm1 + i));
		out.harmPhase[i]  = v (static_cast<Param> (kHarmPhase1 + i));
		out.harmInvert[i] = b (static_cast<Param> (kHarmInv1 + i));

		// Not from the DXi - the per-harmonic LFO it left unfinished.
		out.harmLfoRate[i]  = v (static_cast<Param> (kHarmLfoRate1 + i));
		out.harmLfoDepth[i] = v (static_cast<Param> (kHarmLfoDepth1 + i));

		// The formant block is five consecutive parameters per formant, in
		// the order attack, decay, sustain, release, level - see the enum.
		const int base = kFormantAttack1 + i * 5;
		out.formantAttack[i]  = v (static_cast<Param> (base + 0));
		out.formantDecay[i]   = v (static_cast<Param> (base + 1));
		out.formantSustain[i] = v (static_cast<Param> (base + 2));
		out.formantRelease[i] = v (static_cast<Param> (base + 3));
		out.formantLevel[i]   = v (static_cast<Param> (base + 4));

		out.formantEnable[i] = b (static_cast<Param> (kFormantEnv1 + i));
		out.formantInvert[i] = b (static_cast<Param> (kFormantInv1 + i));
	}

	out.envAttack  = v (kEnvAttack);
	out.envDecay   = v (kEnvDecay);
	out.envSustain = v (kEnvSustain);
	out.envRelease = v (kEnvRelease);
	out.envLevel   = v (kEnvLevel);

	out.filterAttack  = v (kFilterAttack);
	out.filterDecay   = v (kFilterDecay);
	out.filterSustain = v (kFilterSustain);
	out.filterRelease = v (kFilterRelease);
	out.filterLevel   = v (kFilterLevel);

	out.vibratoRate  = v (kVibratoRate);
	out.vibratoDepth = v (kVibratoDepth);
	out.tremoloRate  = v (kTremoloRate);
	out.tremoloDepth = v (kTremoloDepth);

	// Seconds, not the raw control: portamentoSeconds is the one definition
	// of the law, and the panel's readout goes through it too.
	out.portamento = static_cast<float> (portamentoSeconds (internalOf (normalized, kPortamento)));

	out.shapes.lowPass  = static_cast<int> (internalOf (normalized, kLowPassState));
	out.shapes.highPass = static_cast<int> (internalOf (normalized, kHighPassState));
	out.shapes.comb     = static_cast<int> (internalOf (normalized, kCombState));
	out.shapes.grass    = static_cast<int> (internalOf (normalized, kGrassState));
}

//------------------------------------------------------------------------
} // namespace ForTran
