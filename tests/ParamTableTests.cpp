//------------------------------------------------------------------------
// ForTran - parameter table checks
//
// The table is generated from the DXi's own Parameters.h, so what these
// assert is that the generation produced something VST3 can live with:
// dense ids, no null strings (see PORTING-NOTES on RangeParameter), ranges
// that contain their defaults, and normalisation that round-trips.
//
//   c++ -std=c++17 -Wall -Wextra -I<vst3sdk> -I../source ../source/ForTranParams.cpp ParamTableTests.cpp -o param-tests
//------------------------------------------------------------------------

#include "ForTranParams.h"
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <string>
using namespace ForTran;
int fails=0;
static void chk(bool ok,const char*w,int i){ if(!ok){ printf("  FAIL [%d %s]: %s\n",i,kParams[i].title,w); ++fails; } }
int main(){
    printf("parameters: %d\n", (int)kNumParams);
    // 122 from the DXi's own Parameters.h, plus the 20 harmonic LFO controls
    // appended after them. The split is asserted below, where it matters.
    chk(kNumParams==143,"count is not 122 DXi parameters plus 20 LFO plus the output trim",0);
    for(int i=0;i<(int)kNumParams;++i){
        const ParamDef&d=kParams[i];
        chk(d.id==(Steinberg::Vst::ParamID)i,"id does not match its index",i);
        chk(d.title&&d.title[0],"null or empty title",i);
        chk(d.units!=nullptr,"null units (RangeParameter would segfault)",i);
        chk(d.plainMax>d.plainMin,"empty plain range",i);
        chk(d.plainDefault>=d.plainMin&&d.plainDefault<=d.plainMax,"default outside plain range",i);
        double n=d.defaultNormalized();
        chk(n>=0.0&&n<=1.0,"default normalises outside 0..1",i);
        chk(std::fabs(d.toPlain(n)-d.plainDefault)<1e-9,"toPlain(toNormalized(x)) != x",i);
        if(d.type==ParamType::Bool) chk(d.stepCount==1,"bool without stepCount 1",i);
        if(d.type==ParamType::Float) chk(d.stepCount==0,"float with a stepCount",i);
        // internal range must be reachable
        chk(std::fabs(d.toInternal(0.0)-d.internalMin)<1e-9,"toInternal(0) != internalMin",i);
    }
    // High pass, comb and grass offer Off/Add/Mult/Max. Low pass is Off/On -
    // a two-state switch in the DXi's own UI (ForTranPropPage.cpp:352), and
    // the one filter-state parameter whose Parameters.h range was correct.
    const int st[3]={kHighPassState,kCombState,kGrassState};
    for(int j=0;j<3;++j){
        const ParamDef&d=kParams[st[j]];
        chk(d.stepCount==3,"filter state does not have 4 steps",st[j]);
        for(int s=0;s<=3;++s){
            double norm=s/3.0;
            chk((int)d.toInternal(norm)==s,"filter state step does not round-trip",st[j]);
        }
    }
    {
        const ParamDef&d=kParams[kLowPassState];
        chk(d.stepCount==1,"low pass is not a two-state switch",kLowPassState);
        chk((int)d.toInternal(0.0)==0 && (int)d.toInternal(1.0)==1,
            "low pass does not round-trip Off/On",kLowPassState);
    }
    // -- the envelope time controls -------------------------------------
    // 36 of the 122 read as times rather than percentages. What matters is
    // that the three representations agree: the seconds the readout shows,
    // the string it shows them as, and the setting a host gets back when a
    // user types that string in.
    {
        int roles[4]={0,0,0,0};
        for(int i=0;i<(int)kNumParams;++i) roles[(int)envelopeTimeRole(kParams[i].id)]++;
        printf("time controls: %d attack, %d decay, %d release, %d levels\n",
               roles[1],roles[2],roles[3],roles[0]);
        chk(roles[1]==12,"not 12 attack controls",0);
        chk(roles[2]==12,"not 12 decay controls",0);
        chk(roles[3]==12,"not 12 release controls",0);

        // Sustain and level must NOT be times - they are the trap here,
        // sitting in the middle of the formant block.
        chk(envelopeTimeRole(kEnvSustain)==TimeRole::None,"sustain treated as a time",kEnvSustain);
        chk(envelopeTimeRole(kEnvLevel)==TimeRole::None,"level treated as a time",kEnvLevel);
        chk(envelopeTimeRole(kFormantSustain7)==TimeRole::None,"formant sustain treated as a time",kFormantSustain7);
        chk(envelopeTimeRole(kFormantLevel7)==TimeRole::None,"formant level treated as a time",kFormantLevel7);
        chk(envelopeTimeRole(kFilterSustain)==TimeRole::None,"filter sustain treated as a time",kFilterSustain);

        // The factory 20 % must still be the DXi's 0.2 s / 0.4 s.
        chk(std::fabs(envelopeTimeSeconds(kEnvAttack,0.2)-0.2)<1e-9,"default attack is not 200 ms",kEnvAttack);
        chk(std::fabs(envelopeTimeSeconds(kEnvDecay,0.2)-0.2)<1e-9,"default decay is not 200 ms",kEnvDecay);
        chk(std::fabs(envelopeTimeSeconds(kEnvRelease,0.2)-0.4)<1e-9,"default release is not 400 ms",kEnvRelease);

        // Round trip: setting -> seconds -> displayed string -> setting.
        double worst=0.0; char shown[16]={};
        for(int i=0;i<(int)kNumParams;++i){
            if(envelopeTimeRole(kParams[i].id)==TimeRole::None) continue;
            for(double v=0.0; v<=1.0001; v+=0.02){
                const double sec=envelopeTimeSeconds(kParams[i].id,v);
                formatEnvelopeTime(sec,shown,sizeof shown);
                double parsed=0.0;
                chk(parseEnvelopeTime(shown,parsed),"a displayed time will not parse back",i);
                const double back=envelopeTimeToInternal(kParams[i].id,parsed);
                if(std::fabs(back-v)>worst) worst=std::fabs(back-v);
            }
        }
        printf("time round trip: worst setting error %.5f (of full travel)\n", worst);
        // The string is rounded to the millisecond, so an arbitrary setting
        // cannot come back exactly; it must come back close.
        chk(worst<0.001,"a displayed time does not read back to the same setting",0);

        // At the DEFAULT it must be exact, because that is the one value the
        // SDK validator round-trips through getParamStringByValue /
        // getParamValueByString, with a tolerance of 1e-4.
        double worstDefault=0.0;
        for(int i=0;i<(int)kNumParams;++i){
            const ParamDef&d=kParams[i];
            if(envelopeTimeRole(d.id)==TimeRole::None) continue;
            const double norm=d.defaultNormalized();
            formatEnvelopeTime(envelopeTimeSeconds(d.id,norm),shown,sizeof shown);
            double parsed=0.0; parseEnvelopeTime(shown,parsed);
            const double back=envelopeTimeToInternal(d.id,parsed);
            if(std::fabs(back-norm)>worstDefault) worstDefault=std::fabs(back-norm);
        }
        printf("at the default setting: worst error %.3g (validator tolerance 1e-4)\n", worstDefault);
        chk(worstDefault<1e-4,"the default does not round-trip - the validator will warn",0);

        // The units a user might type.
        double sec=0.0;
        chk(parseEnvelopeTime("200 ms",sec)&&std::fabs(sec-0.2)<1e-12,"\"200 ms\" misread",0);
        chk(parseEnvelopeTime("200ms",sec)&&std::fabs(sec-0.2)<1e-12,"\"200ms\" misread",0);
        chk(parseEnvelopeTime("1.25 s",sec)&&std::fabs(sec-1.25)<1e-12,"\"1.25 s\" misread",0);
        chk(parseEnvelopeTime("1.25",sec)&&std::fabs(sec-1.25)<1e-12,"a bare number is not seconds",0);
        chk(parseEnvelopeTime("2 SEC",sec)&&std::fabs(sec-2.0)<1e-12,"\"2 SEC\" misread",0);
        chk(!parseEnvelopeTime("fast",sec),"a non-numeric string was accepted",0);

        // Out of range typing must clamp, not wrap.
        chk(envelopeTimeToInternal(kEnvAttack,999.0)==1.0,"a huge time does not clamp to full travel",0);
        chk(envelopeTimeToInternal(kEnvAttack,-1.0)==0.0,"a negative time does not clamp to zero",0);

        // 1000 ms must print as "1.00 s", never as "1000 ms".
        formatEnvelopeTime(0.9999,shown,sizeof shown);
        printf("0.9999 s prints as \"%s\"\n", shown);
        chk(std::string(shown)=="1.00 s","the ms/s threshold rounds up into \"1000 ms\"",0);
    }

    // -- the ten harmonic phase controls --------------------------------
    // They are a real -90..+90 degree phase shift and read as degrees. What
    // matters is that the degrees agree with what the DSP applies, because
    // a readout that disagreed with the sound would be worse than the
    // percentage it replaced.
    {
        int n = 0;
        for (int i = 0; i < (int)kNumParams; ++i) if (isPhaseParam(kParams[i].id)) ++n;
        printf("phase controls: %d\n", n);
        chk(n == 10, "not 10 phase controls", 0);
        chk(!isPhaseParam(kEnvAttack), "a time control is treated as a phase", 0);
        chk(!isPhaseParam(kHarm1), "a level control is treated as a phase", 0);

        // The DXi's own expression, verbatim, is the reference.
        const double PI = 3.14159265358979323846;
        double worst = 0.0;
        for (double c = 0.0; c <= 1.0001; c += 0.01) {
            const double dxiRadians = -PI * (0.5 - c);           // ForTran.cpp:791
            const double shownDegrees = phaseDegrees(c);
            worst = std::fmax(worst, std::fabs(shownDegrees - dxiRadians * 180.0 / PI));
        }
        printf("phase readout vs the DXi's own expression: worst error %.3g degrees\n", worst);
        chk(worst < 1e-9, "the degrees shown are not what the DSP applies", 0);

        chk(std::fabs(phaseDegrees(0.0) + 90.0) < 1e-9, "0% is not -90 degrees", 0);
        chk(std::fabs(phaseDegrees(0.5)) < 1e-9, "the centre is not 0 degrees", 0);
        chk(std::fabs(phaseDegrees(1.0) - 90.0) < 1e-9, "100% is not +90 degrees", 0);
        chk(std::fabs(phaseDegrees(kParams[kHarmPhase1].defaultNormalized())) < 1e-9,
            "the default is not 0 degrees", kHarmPhase1);

        // Round trip, and the sign of zero.
        double worstBack = 0.0;
        char shown[8] = {};
        for (double c = 0.0; c <= 1.0001; c += 0.01) {
            formatPhase(phaseDegrees(c), shown, sizeof shown);
            worstBack = std::fmax(worstBack, std::fabs(phaseToInternal(atof(shown)) - c));
        }
        printf("phase round trip: worst setting error %.4f\n", worstBack);
        chk(worstBack < 0.004, "a displayed phase does not read back", 0);

        formatPhase(phaseDegrees(0.5), shown, sizeof shown);
        printf("the centre displays as \"%s\"\n", shown);
        chk(std::string(shown) == "0", "the centre does not display as a plain 0", 0);
        formatPhase(phaseDegrees(0.75), shown, sizeof shown);
        chk(std::string(shown) == "+45", "75% does not display as +45", 0);
        formatPhase(phaseDegrees(0.25), shown, sizeof shown);
        chk(std::string(shown) == "-45", "25% does not display as -45", 0);
        // A value that rounds to zero from below must not read "-0".
        formatPhase(-0.2, shown, sizeof shown);
        chk(std::string(shown) == "0", "a value rounding to zero displays as -0", 0);

        // Exact at the default, which is the value the SDK validator
        // round-trips through getParamStringByValue / getParamValueByString.
        for (int i = 0; i < (int)kNumParams; ++i) {
            if (!isPhaseParam(kParams[i].id)) continue;
            const double norm = kParams[i].defaultNormalized();
            formatPhase(phaseDegrees(norm), shown, sizeof shown);
            chk(std::fabs(phaseToInternal(atof(shown)) - norm) < 1e-4,
                "the default phase does not round-trip - the validator will warn", i);
        }

        chk(phaseToInternal(999.0) == 1.0, "a huge angle does not clamp", 0);
        chk(phaseToInternal(-999.0) == 0.0, "a huge negative angle does not clamp", 0);
    }

    // -- the twenty harmonic LFO controls -------------------------------
    // These are the only parameters in the table with no DXi counterpart.
    // What matters is that they were APPENDED - if they had been inserted,
    // every id after them would have shifted and every saved project would
    // load the wrong values into the wrong controls.
    {
        printf("harmonic LFO: ids %d..%d and %d..%d, of %d\n",
               kHarmLfoRate1, kHarmLfoRate10, kHarmLfoDepth1, kHarmLfoDepth10, (int)kNumParams);
        chk(kNumParams == 143, "the table is not 122 DXi + 20 LFO + 1 trim", 0);

        // Every DXi parameter must still be where it was.
        chk(kEnable == 0, "kEnable moved", 0);
        chk(kHarmInv10 == 121, "the last DXi parameter moved - saved projects would break", 0);
        chk(kHarmLfoRate1 == 122, "the LFO block does not start straight after the DXi's", 0);
        chk(kHarmLfoDepth1 == kHarmLfoRate10 + 1, "the depth block does not follow the rate block", 0);
        chk(kOutputLevel == kHarmLfoDepth10 + 1, "the output trim is not after the LFO block", 0);
        chk(kOutputLevel == (int)kNumParams - 1, "the output trim is not last", 0);

        // Depth must default to OFF, or adding the feature would change
        // every existing sound.
        for (int n = 0; n < 10; ++n) {
            const ParamDef& d = kParams[kHarmLfoDepth1 + n];
            chk(d.plainDefault == 0.0, "an LFO depth does not default to off", kHarmLfoDepth1 + n);
            chk(d.defaultNormalized() == 0.0, "an LFO depth's normalized default is not zero",
                kHarmLfoDepth1 + n);
        }

        // Rate defaults carry the DXi's own intended seeding: harmonic n at
        // 1/n Hz. Checked here as the CONTROL value; VoiceTests checks the
        // Hz the mapping turns it into, since that lives in Voice.h.
        for (int n = 1; n <= 10; ++n) {
            const double want = std::log((1.0/n)/0.05) / std::log(20.0/0.05);
            const double got = kParams[kHarmLfoRate1 + n - 1].defaultNormalized();
            if (n <= 2 || n == 10)
                printf("   harmonic %2d default rate control %.4f (want %.4f)\n", n, got, want);
            chk(std::fabs(got - want) < 5e-5, "an LFO rate default is not 1/n Hz", kHarmLfoRate1+n-1);
        }

        // They are not times and not phases, or they would be formatted as
        // such by the display code those predicates gate.
        for (int i = kHarmLfoRate1; i <= kHarmLfoDepth10; ++i) {
            chk(envelopeTimeRole(kParams[i].id) == TimeRole::None, "an LFO control is treated as a time", i);
            chk(!isPhaseParam(kParams[i].id), "an LFO control is treated as a phase", i);
        }
    }

    // -- the output trim --------------------------------------------------
    // The one control the DXi had no equivalent of at all. What matters is
    // that the TOP of it is the DXi's own staging, so nothing is lost, and
    // that the default is well below it, because the original runs above
    // unity on a single note.
    {
        printf("output trim: 0%%=%.4f  25%%=%.4f  50%%=%.4f  75%%=%.4f  100%%=%.4f\n",
               outputGain(0.0), outputGain(0.25), outputGain(0.5),
               outputGain(0.75), outputGain(1.0));
        chk(outputGain(1.0) == 1.0, "the top of the trim is not unity - the DXi's staging is unreachable", 0);
        chk(outputGain(0.0) == 0.0, "the bottom of the trim is not silence", 0);
        chk(std::fabs(outputDecibels(0.5) + 20.0) < 1e-9, "the centre of the trim is not -20 dB", 0);
        chk(std::fabs(kParams[kOutputLevel].defaultNormalized() - 0.5) < 1e-9,
            "the trim does not default to its centre", kOutputLevel);

        // Monotonic, and round-trips through the displayed decibels.
        double prev = -1.0, worst = 0.0;
        for (double c = 0.0; c <= 1.0001; c += 0.01) {
            const double g = outputGain(c);
            chk(g >= prev, "the trim is not monotonic", 0);
            prev = g;
            if (c > 0.005) worst = std::fmax(worst, std::fabs(outputControlFromDb(outputDecibels(c)) - c));
        }
        printf("trim round trip through dB: worst error %.3g\n", worst);
        chk(worst < 1e-9, "a displayed trim does not read back", 0);
        chk(outputControlFromDb(99.0) == 1.0, "a huge gain does not clamp to the top", 0);
        chk(outputControlFromDb(-999.0) == 0.0, "a huge cut does not clamp to silence", 0);
    }

    printf("kEnable default = %.0f (DXi shipped this ON)\n", kParams[kEnable].plainDefault);
    printf(fails?"\n%d CHECK(S) FAILED\n":"\nall checks passed\n",fails);
    return fails!=0;
}
