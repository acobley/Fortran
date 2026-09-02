//------------------------------------------------------------------------
// ForTran - filter response checks
//
//   c++ -std=c++17 -O2 -Wall -Wextra -I../source ../source/FilterInfo.cpp FilterInfoTests.cpp -o filter-tests
//
// As with the ADSR, the 2003 class is transcribed below so the port can be
// diffed against it bin for bin rather than merely inspected.
//------------------------------------------------------------------------

#include "FilterInfo.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>

using namespace ForTran;
static int fails = 0;
static void chk (bool ok, const char* what)
{ if (!ok) { std::printf("  FAIL: %s\n", what); ++fails; } }

//------------------------------------------------------------------------
// FilterInfo.cpp, 2003, transcribed.
struct OriginalFilterInfo
{
	float LowPassMagnitude[552], LowPassTheta[552];
	float HighPassMagnitude[552], HighPassTheta[552];
	float CombMagnitude[552], CombTheta[552];
	float GrassMagnitude[552], GrassTheta[552];
	float Frequencys[552];
	float Cutoff, Feedback, Poles;

	OriginalFilterInfo ()
	{
		double const dStep = std::pow (2.0, 1.0 / 48.0);
		Frequencys[69*4] = 440;
		for (int ix = 277; ix < 69*8; ++ix) Frequencys[ix] = Frequencys[ix-1] * dStep;
		for (int ix = 275; ix >= 0; --ix)   Frequencys[ix] = Frequencys[ix+1] / dStep;
		Feedback = 0.5f; Poles = 4; Cutoff = Frequencys[69*4];
		CalculateFilters ();
	}
	void SetFeedback (float f) { Feedback = f; }
	void SetPoles (float p)    { Poles = p; }
	void SetCutoff (float f)   { Cutoff = Frequencys[(int)(f * 552.0)]; }   // OOB at f == 1
	void CalculateFilters ()
	{
		static const double dPI = 3.14159265358979323846;
		double dDivPoles = 1 + Poles/5; (void)dDivPoles;
		float CombPoles = 2.5f/10*Poles;
		for (int ix = 0; ix < 552; ix++)
		{
			double dLowPassRatio = Frequencys[ix]/Cutoff;
			double dRadiusLowPass = sqrt(1.0+dLowPassRatio*dLowPassRatio);
			double dTheataLowPass = atan(dLowPassRatio);
			double dPowerRealLowPass = pow(dRadiusLowPass,Poles)*cos(Poles*dTheataLowPass);
			double dPowerImaginaryLowPass = pow(dRadiusLowPass,Poles)*sin(Poles*dTheataLowPass);
			double dKrLowpass = Feedback+dPowerRealLowPass;
			double dRRLowpass = dKrLowpass/(dKrLowpass*dKrLowpass+dPowerImaginaryLowPass*dPowerImaginaryLowPass);
			double dRILowpass = -1.0*dPowerImaginaryLowPass/(dKrLowpass*dKrLowpass+dPowerImaginaryLowPass*dPowerImaginaryLowPass);
			LowPassMagnitude[ix] = sqrt(dRRLowpass*dRRLowpass+dRILowpass*dRILowpass);
			if (LowPassMagnitude[ix]>1.0) LowPassMagnitude[ix]=1.0;
			LowPassTheta[ix] = atan(dRILowpass/dRRLowpass);

			double dHighPassRatio = Cutoff/Frequencys[ix];
			double dRadiusHighPass = sqrt(1.0+dHighPassRatio*dHighPassRatio);
			double dTheataHighPass = atan(dHighPassRatio);
			double dPowerRealHighPass = pow(dRadiusHighPass,Poles)*cos(Poles*dTheataHighPass);
			double dPowerImaginaryHighPass = pow(dRadiusHighPass,Poles)*sin(Poles*dTheataHighPass);
			double dKrHighpass = Feedback+dPowerRealHighPass;
			double dRRHighpass = dKrHighpass/(dKrHighpass*dKrHighpass+dPowerImaginaryHighPass*dPowerImaginaryHighPass);
			double dRIHighpass = -1.0*dPowerImaginaryHighPass/(dKrHighpass*dKrHighpass+dPowerImaginaryHighPass*dPowerImaginaryHighPass);
			HighPassMagnitude[ix] = sqrt(dRRHighpass*dRRHighpass+dRIHighpass*dRIHighpass);
			if (HighPassMagnitude[ix]>1.0) HighPassMagnitude[ix]=1.0;
			HighPassTheta[ix] = atan(dRIHighpass/dRRHighpass);

			double CombRatio = (float)ix/48.0;
			double RawSin = sin(dPI*CombRatio*CombPoles);
			double AbsSin = fabs(RawSin);
			CombMagnitude[ix] = pow(AbsSin,(Feedback/4.0));
			if (CombMagnitude[ix]>1.0) CombMagnitude[ix]=1.0;
			CombTheta[ix] = dPI*RawSin;
			GrassMagnitude[ix] = 1.0-CombMagnitude[ix];
			if (GrassMagnitude[ix]>1.0) GrassMagnitude[ix]=1.0;
			GrassTheta[ix] = dPI*RawSin;
		}
	}
};

//------------------------------------------------------------------------
int main ()
{
	// 1. Defaults: the state the shipped DXi actually ran in, because
	//    CalculateFilters was only ever re-run from the property page.
	std::printf("1. default tables (cutoff 440, feedback 0.5, poles 4)\n");
	{
		OriginalFilterInfo o;
		FilterInfo p;
		double worst = 0.0; int nonFinite = 0;
		const char* names[4] = {"low pass","high pass","comb","grass"};
		const float* op[4] = {o.LowPassMagnitude,o.HighPassMagnitude,o.CombMagnitude,o.GrassMagnitude};
		FilterInfo::Shape sh[4] = {FilterInfo::Shape::LowPass,FilterInfo::Shape::HighPass,
		                           FilterInfo::Shape::Comb,FilterInfo::Shape::Grass};
		for (int s = 0; s < 4; ++s)
		{
			const float* pp = p.magnitudeCurve(sh[s]);
			double w = 0.0;
			for (int i = 0; i < 552; ++i)
			{
				if (!std::isfinite(pp[i])) ++nonFinite;
				w = std::fmax(w, std::fabs((double)op[s][i] - (double)pp[i]));
			}
			std::printf("   %-10s max deviation %g\n", names[s], w);
			worst = std::fmax(worst, w);
		}
		chk(worst == 0.0, "the default tables are not identical to the original");
		chk(nonFinite == 0, "non-finite magnitudes in the port");
	}

	// 2. Across the control ranges the port now exposes (they were dead in
	//    the DXi unless the editor was open - see PORTING-NOTES section 6).
	std::printf("2. across cutoff x gain x poles, now that the controls are live\n");
	{
		double worst = 0.0; int cases = 0, nonFinite = 0;
		for (float cut : {0.0f, 0.1f, 0.25f, 0.5f, 0.75f, 0.99f})
			for (float gain : {0.0f, 0.25f, 0.5f, 1.0f})
				for (float pol : {0.0f, 0.2f, 0.5f, 1.0f})
				{
					OriginalFilterInfo o;
					o.SetCutoff(cut);
					o.SetFeedback(gain * 3.9f);
					o.SetPoles((float)(int)(1 + pol * 10));
					o.CalculateFilters();

					FilterInfo p;
					p.setCutoff(cut);
					p.setFeedback(gain * 3.9f);
					p.setPoles(1 + pol * 10);
					p.calculate();

					const float* op[4] = {o.LowPassMagnitude,o.HighPassMagnitude,o.CombMagnitude,o.GrassMagnitude};
					FilterInfo::Shape sh[4] = {FilterInfo::Shape::LowPass,FilterInfo::Shape::HighPass,
					                           FilterInfo::Shape::Comb,FilterInfo::Shape::Grass};
					for (int s = 0; s < 4; ++s)
					{
						const float* pp = p.magnitudeCurve(sh[s]);
						for (int i = 0; i < 552; ++i)
						{
							if (!std::isfinite(pp[i])) ++nonFinite;
							worst = std::fmax(worst, std::fabs((double)op[s][i] - (double)pp[i]));
						}
					}
					++cases;
				}
		std::printf("   %d settings x 4 shapes x 552 bins, max deviation %g, %d non-finite\n",
		            cases, worst, nonFinite);
		chk(worst == 0.0, "the port diverges from the original inside the control range");
		chk(nonFinite == 0, "non-finite magnitudes in the port");
	}

	// 3. Cutoff at the very top indexes one past the end in the original.
	std::printf("3. cutoff = 1.0 - the top of the control\n");
	{
		FilterInfo p;
		p.setCutoff(1.0f); p.calculate();
		int bad = 0;
		for (int s = 0; s < 4; ++s)
		{
			const float* pp = p.magnitudeCurve((FilterInfo::Shape)s);
			for (int i = 0; i < 552; ++i) if (!std::isfinite(pp[i])) ++bad;
		}
		std::printf("   port: %d non-finite (the original reads Frequencys[552], off the end)\n", bad);
		chk(bad == 0, "cutoff at maximum produces non-finite output");
	}

	// 4. The harmonic lookup must stay inside the table for every reachable
	//    index. The caller clamps to 0..276; the guard lives here anyway.
	std::printf("4. harmonic lookup stays in range for every bin 0..551\n");
	{
		FilterInfo p;
		int bad = 0;
		for (int bin = 0; bin <= 551; ++bin)
			for (int s = 0; s < 4; ++s)
			{
				float m[10], t[10];
				p.values((FilterInfo::Shape)s, bin, m, t);
				for (int j = 0; j < 10; ++j)
					if (!std::isfinite(m[j]) || !std::isfinite(t[j]) || m[j] < 0.f || m[j] > 1.f) ++bad;
			}
		std::printf("   %d bad values across 552 bins x 4 shapes x 10 harmonics\n", bad);
		chk(bad == 0, "harmonic lookup produced a bad value");
	}

	// 5. The grid: bin 276 is 440 Hz and the step is a quarter-semitone.
	std::printf("5. the frequency grid\n");
	{
		FilterInfo p;
		const double step = std::pow(2.0, 1.0/48.0);
		std::printf("   bin 276 = %.4f Hz, bin 0 = %.4f Hz, bin 551 = %.1f Hz\n",
		            p.binFrequency(276), p.binFrequency(0), p.binFrequency(551));
		chk(std::fabs(p.binFrequency(276) - 440.0) < 1e-3, "bin 276 is not 440 Hz");
		chk(std::fabs(p.binFrequency(276+48) / p.binFrequency(276) - 2.0) < 1e-3,
		    "48 bins is not one octave");
		chk(std::fabs(p.binFrequency(277) / p.binFrequency(276) - step) < 1e-5,
		    "the step is not 2^(1/48)");
	}

	// 6. PHASE. The magnitudes above are bit-identical to the original; the
	//    phases deliberately are NOT. The DXi wrote atan(RI/RR), which has
	//    thrown the quadrant away before atan sees it and so jumps by pi
	//    wherever the real part changes sign. The port uses atan2.
	//
	//    The invariant that says the change is EXACTLY a quadrant fix and
	//    nothing else: every phase is either identical to the original's or
	//    differs from it by pi.
	std::printf("6. phase: atan2 against the original's atan, must differ by 0 or pi\n");
	{
		int compared = 0, other = 0; double worstResidual = 0.0;
		int flipped = 0;
		for (float cut : {0.1f, 0.5f, 0.99f})
			for (float gain : {0.0f, 0.5f, 1.0f})
				for (float pol : {0.0f, 0.5f, 1.0f})
				{
					OriginalFilterInfo o;
					o.SetCutoff(cut); o.SetFeedback(gain*3.9f); o.SetPoles((float)(int)(1+pol*10));
					o.CalculateFilters();
					FilterInfo p;
					p.setCutoff(cut); p.setFeedback(gain*3.9f); p.setPoles(1.f+pol*10.f);
					if (p.needsRecalculation()) p.calculate();

					const float* ot[2] = { o.LowPassTheta, o.HighPassTheta };
					FilterInfo::Shape sh[2] = { FilterInfo::Shape::LowPass, FilterInfo::Shape::HighPass };
					for (int shape = 0; shape < 2; ++shape)
						for (int bin = 0; bin < 552; ++bin)
						{
							float m[10], t[10];
							p.values(sh[shape], bin, m, t);   // partial 1 sits on the bin itself
							const double d = (double)t[0] - (double)ot[shape][bin];
							// 0 or +-pi, and nothing else.
							double r = std::fabs(d);
							if (r > 1.5) r = std::fabs(r - M_PI);
                            if (std::fabs(d) > 1.5) ++flipped;
							worstResidual = std::fmax(worstResidual, r);
							if (r > 1e-4) ++other;
							++compared;
						}
				}
		std::printf("   %d phases compared, %d differ by pi (a quadrant), worst residual %.3g rad\n",
		            compared, flipped, worstResidual);
		chk(other == 0, "a phase differs from the original by something other than 0 or pi");
		chk(flipped > 0, "no phase differs at all - the atan2 change is not in effect");
	}

	// 7. And the point of it: no phase discontinuity across the bins a
	//    filter envelope sweep actually visits. Measured as the change in a
	//    partial's COMPLEX amplitude, since a flip on a silent partial is
	//    silent. With the original's atan this reaches 2.0 - a partial at
	//    full magnitude going from +1 to -1 in one sample, which is the
	//    click.
	std::printf("7. no sign flips across the swept bins\n");
	{
		double worstPort = 0.0, worstOriginal = 0.0;
		for (float gain : {0.0f, 0.25f, 0.5f, 0.75f})
			for (float pol : {0.25f, 0.5f, 0.75f})
			{
				OriginalFilterInfo o;
				o.SetCutoff(0.5f); o.SetFeedback(gain*3.9f); o.SetPoles((float)(int)(1+pol*10));
				o.CalculateFilters();
				FilterInfo p;
				p.setCutoff(0.5f); p.setFeedback(gain*3.9f); p.setPoles(1.f+pol*10.f);
				if (p.needsRecalculation()) p.calculate();

				FilterInfo::ShapeStates st; st.lowPass = 1;
				float m0[10],t0[10],m1[10],t1[10];
				for (int b = 1; b < 276; ++b)
				{
					p.combine(st, b, m0, t0); p.combine(st, b+1, m1, t1);
					for (int k = 0; k < 10; ++k)
					{
						const double dx = m1[k]*std::cos(t1[k]) - m0[k]*std::cos(t0[k]);
						const double dy = m1[k]*std::sin(t1[k]) - m0[k]*std::sin(t0[k]);
						worstPort = std::fmax(worstPort, std::sqrt(dx*dx+dy*dy));
					}
					// the same measure on the original's phases
					for (int k = 0; k < 10; ++k)
					{
						const int b0 = b, b1 = b+1;
						const double a0 = o.LowPassTheta[b0], a1 = o.LowPassTheta[b1];
						const double g0 = o.LowPassMagnitude[b0], g1 = o.LowPassMagnitude[b1];
						const double dx = g1*std::cos(a1) - g0*std::cos(a0);
						const double dy = g1*std::sin(a1) - g0*std::sin(a0);
						worstOriginal = std::fmax(worstOriginal, std::sqrt(dx*dx+dy*dy));
					}
				}
			}
		std::printf("   worst inter-bin jump: original %.3f, port %.3f\n", worstOriginal, worstPort);
		chk(worstOriginal > 1.5, "the original does not show the flip - check the transcription");
		chk(worstPort < 1.2, "the port still has a large phase discontinuity across a sweep");
	}

	std::printf(fails ? "\n%d CHECK(S) FAILED\n" : "\nall checks passed\n", fails);
	return fails != 0;
}
