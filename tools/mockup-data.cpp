//------------------------------------------------------------------------
// ForTran - the panel's state, as JSON, for tools/make-mockup.py
//
// The mock-up has to show what the editor would show, not something that
// looks about right, so everything it draws that is COMPUTED comes from the
// real code: the parameter table's own defaults, FilterInfo for the response
// curve and the partial magnitudes behind the waveform, and traceEnvelope
// driving the real Adsr for the three envelope panels. This file is a
// transcription of ForTranEditor::refreshDisplays and refreshEnvelopes with
// the controller replaced by "every parameter at its default".
//
//   c++ -std=c++17 -O2 -I<vst3sdk> -I../source ../source/ForTranParams.cpp
//       ../source/FilterInfo.cpp ../source/Adsr.cpp mockup-data.cpp -o mockup-data
//   (one command; make-mockup.py builds it for you)
//------------------------------------------------------------------------

#include "ForTranParams.h"
#include "FilterInfo.h"
#include "Adsr.h"

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace ForTran;
using Steinberg::Vst::ParamID;

namespace {

constexpr double kPI = 3.14159265358979323846;

/** Every parameter sits at its default - this is a fresh instance. */
double normalizedOf (ParamID id) { return paramDef (id).defaultNormalized (); }
double internalOf   (ParamID id) { return paramDef (id).toInternal (normalizedOf (id)); }

void printArray (const char* name, const float* data, int count, bool last = false)
{
    std::printf ("  \"%s\": [", name);
    for (int i = 0; i < count; ++i)
        std::printf ("%s%.6g", i ? "," : "", (double) data[i]);
    std::printf ("]%s\n", last ? "" : ",");
}

} // namespace

int main ()
{
    std::printf ("{\n");

    // ---- the ids the mock-up needs, by name ------------------------------
    // Emitted rather than counted out by hand on the Python side: the enum
    // is the one place that knows where each block starts.
    std::printf ("  \"ids\": {");
    struct { const char* name; ParamID id; } kIds[] = {
        { "harm1", kHarm1 }, { "harmPhase1", kHarmPhase1 }, { "harmInv1", kHarmInv1 },
        { "envAttack", kEnvAttack }, { "envDecay", kEnvDecay },
        { "envSustain", kEnvSustain }, { "envRelease", kEnvRelease },
        { "envLevel", kEnvLevel },
        { "filterAttack", kFilterAttack }, { "filterDecay", kFilterDecay },
        { "filterSustain", kFilterSustain }, { "filterRelease", kFilterRelease },
        { "filterLevel", kFilterLevel },
        { "vibratoRate", kVibratoRate }, { "vibratoDepth", kVibratoDepth },
        { "tremoloRate", kTremoloRate }, { "tremoloDepth", kTremoloDepth },
        { "cutoff", kCutoff }, { "filterGain", kFilterGain }, { "poles", kPoles },
        { "lowPassState", kLowPassState }, { "highPassState", kHighPassState },
        { "combState", kCombState }, { "grassState", kGrassState },
        { "formantAttack1", kFormantAttack1 }, { "formantEnv1", kFormantEnv1 },
        { "formantInv1", kFormantInv1 },
        { "outputLevel", kOutputLevel }, { "pan", kPan }, { "ladderEnable", kLadderEnable },
    };
    for (std::size_t i = 0; i < sizeof (kIds) / sizeof (kIds[0]); ++i)
        std::printf ("%s\"%s\": %d", i ? ", " : "", kIds[i].name, (int) kIds[i].id);
    std::printf ("},\n");

    // ---- every parameter's default, keyed by id --------------------------
    std::printf ("  \"defaults\": {");
    for (int i = 0; i < (int) kNumParams; ++i)
        std::printf ("%s\"%d\": %.6g", i ? ", " : "", i, normalizedOf ((ParamID) i));
    std::printf ("},\n");

    // ---- the filter, exactly as refreshDisplays sets it up ---------------
    FilterInfo filter;
    FilterInfo::ShapeStates states;
    states.lowPass  = (int) internalOf (kLowPassState);
    states.highPass = (int) internalOf (kHighPassState);
    states.comb     = (int) internalOf (kCombState);
    states.grass    = (int) internalOf (kGrassState);

    filter.setCutoff   ((float) internalOf (kCutoff));
    filter.setFeedback ((float) internalOf (kFilterGain) * 3.9f);
    filter.setPoles    (1.f + (float) internalOf (kPoles) * 10.f);
    if (filter.needsRecalculation ())
        filter.calculate ();

    float curve[FilterInfo::kNumBins];
    filter.combinedCurve (states, curve);
    printArray ("response", curve, FilterInfo::kNumBins);

    // ---- the waveform panel: ten partials at bin 276 (440 Hz) ------------
    float magnitude[FilterInfo::kNumParts], theta[FilterInfo::kNumParts];
    filter.combine (states, 276, magnitude, theta);

    float level[10], phase[10];
    for (int k = 0; k < 10; ++k)
    {
        const double lv  = internalOf ((ParamID) (kHarm1 + k));
        const bool   inv = internalOf ((ParamID) (kHarmInv1 + k)) >= 0.5;
        level[k] = (float) (inv ? -lv : lv);
        phase[k] = (float) (-kPI * (0.5 - internalOf ((ParamID) (kHarmPhase1 + k))));
    }

    constexpr int kWavePoints = 256;
    float cycle[kWavePoints];
    for (int i = 0; i < kWavePoints; ++i)
    {
        const double t = 2.0 * kPI * i / (kWavePoints - 1);
        double sum = 0.0;
        for (int k = 0; k < 10; ++k)
            sum += level[k] * std::sin ((k + 1) * t + theta[k] + phase[k]) * magnitude[k];
        cycle[i] = (float) sum;
    }
    printArray ("wave", cycle, kWavePoints);

    // ---- the three envelope panels, through the real Adsr ----------------
    // Formant 1 is the selected one on a freshly opened editor.
    constexpr int kEnvPoints = 220;
    const char* const names[3] = { "Amp Env", "Filter Env", "Harmonic 1" };

    std::printf ("  \"envelopes\": [\n");
    for (int which = 0; which < 3; ++which)
    {
        EnvelopeSpec spec;
        if (which == 0)
        {
            spec.attack  = (float) internalOf (kEnvAttack);
            spec.decay   = (float) internalOf (kEnvDecay);
            spec.sustain = (float) internalOf (kEnvSustain);
            spec.release = (float) internalOf (kEnvRelease);
            spec.level   = (float) internalOf (kEnvLevel);
        }
        else if (which == 1)
        {
            spec.attack  = (float) internalOf (kFilterAttack);
            spec.decay   = (float) internalOf (kFilterDecay);
            spec.sustain = (float) internalOf (kFilterSustain);
            spec.release = (float) internalOf (kFilterRelease);
            spec.level   = (float) internalOf (kFilterLevel);
        }
        else
        {
            spec.attack  = (float) internalOf (kFormantAttack1);
            spec.decay   = (float) internalOf (kFormantDecay1);
            spec.sustain = (float) internalOf (kFormantSustain1);
            spec.release = (float) internalOf (kFormantRelease1);
            spec.level   = (float) internalOf (kFormantLevel1);
            spec.enabled = internalOf (kFormantEnv1) >= 0.5;
            spec.invert  = internalOf (kFormantInv1) >= 0.5;
        }

        float shape[kEnvPoints];
        const double seconds = traceEnvelope (spec, shape, kEnvPoints);

        char annotation[24] = {};
        if (!spec.enabled)
            std::snprintf (annotation, sizeof (annotation), "off");
        else
            formatEnvelopeTime (seconds, annotation, sizeof (annotation));

        std::printf ("    { \"caption\": \"%s\", \"annotation\": \"%s\", \"data\": [",
                     names[which], annotation);
        for (int i = 0; i < kEnvPoints; ++i)
            std::printf ("%s%.6g", i ? "," : "", (double) shape[i]);
        std::printf ("] }%s\n", which == 2 ? "" : ",");
    }
    std::printf ("  ],\n");

    // ---- the four shape selectors' text ----------------------------------
    const int lp = (int) internalOf (kLowPassState);
    std::printf ("  \"shapes\": [\"%s\", \"%s\", \"%s\", \"%s\"]\n",
                 kLowPassStateNames[lp < 0 ? 0 : (lp > 1 ? 1 : lp)],
                 kFilterStateNames[(int) internalOf (kHighPassState)],
                 kFilterStateNames[(int) internalOf (kCombState)],
                 kFilterStateNames[(int) internalOf (kGrassState)]);

    std::printf ("}\n");
    return 0;
}
