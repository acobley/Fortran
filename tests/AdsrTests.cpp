//------------------------------------------------------------------------
// ForTran - ADSR checks
//
// Adsr.cpp has no SDK dependency, so this runs anywhere:
//
//   c++ -std=c++17 -O2 -Wall -Wextra -I../source ../source/Adsr.cpp AdsrTests.cpp -o adsr-tests
//
// The original DXi ADSR is reproduced verbatim below as `OriginalAdsr` so
// the port can be diffed against it sample for sample. That is what makes
// the deliberate changes checkable rather than merely plausible.
//
// There are two of them, and the tests separate them:
//
//   * the attack/decay/release counts are floored at one millisecond, so a
//     control at the bottom of its travel no longer divides by zero;
//   * the control is squared and the range widened to 5 s / 10 s.
//
// The second changes only how a control value becomes a SAMPLE COUNT. So
// test 1 hands the original the port's own counts and requires the two to
// agree bit for bit - the envelope's shape is untouched - and test 1b
// pins the mapping itself, checking that the factory 20 % setting still
// lands on exactly the counts the DXi's linear 0..1 s / 0..2 s produced.
//------------------------------------------------------------------------

#include "Adsr.h"
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <vector>

using namespace ForTran;

static int fails = 0;
static void chk (bool ok, const char* what)
{
	if (!ok) { std::printf("  FAIL: %s\n", what); ++fails; }
}

//------------------------------------------------------------------------
// ADSR.cpp, 2003, transcribed. Note `long` holding the times, and no guard
// anywhere against those times being zero.
struct OriginalAdsr
{
	long Attack = 0, Decay = 0, Release = 0;      // truncated from 0.0005 etc
	float Sustain = 0.50f;
	int Phase = 0;                                 // 0=A 1=D 2=S 3=R
	float CurrentValue = 0.f;
	long SamplesPerSec = 44100;
	float EnvLevel = 1.f;
	bool EnvEnable = false, Invert = false;
	long CurrentSample = 0, SustainSample = 0;
	float ReleaseInitialValue = 0.f;

	void SetAttackTime (float r)  { Attack  = (long)(r * (1.0 * SamplesPerSec)); }
	void SetDecayTime (float r)   { Decay   = (long)(r * (1.0 * SamplesPerSec)); }
	void SetSustainLevel (float r){ Sustain = r; ReleaseInitialValue = Sustain; }
	void SetReleaseTime (float r) { Release = (long)(r * (2.0 * SamplesPerSec)); }
	void SetRelease ()
	{
		if (Phase != 3) { SustainSample = CurrentSample; ReleaseInitialValue = CurrentValue; }
		Phase = 3;
	}
	float GetValue ()
	{
		float value = CurrentValue;
		switch (Phase)
		{
			case 0: value = (float)CurrentSample / (float)Attack;
			        if ((CurrentSample > Attack) || (value > 1.0)) Phase = 1;
			        break;
			case 1: value = 1.0f - (((float)CurrentSample - Attack) / (float)Decay);
			        if (value <= Sustain) Phase = 2;
			        break;
			case 2: break;
			case 3: value = ReleaseInitialValue - (((float)CurrentSample - SustainSample) / (float)Release);
			        if (value < 0.0f) value = 0.0f;
			        break;
		}
		CurrentValue = value;
		CurrentSample++;
		if (EnvEnable == false) return 1.0f;
		if (Invert == true) return EnvLevel * (1.0f - value);
		return EnvLevel * value;
	}
};

//------------------------------------------------------------------------
static std::vector<float> runOriginal (float a, float d, float s, float r,
                                       int n, int noteOffAt, double sr)
{
	OriginalAdsr e;
	e.SamplesPerSec = (long)sr; e.EnvEnable = true; e.EnvLevel = 1.f;
	e.SetAttackTime(a); e.SetDecayTime(d); e.SetSustainLevel(s); e.SetReleaseTime(r);
	std::vector<float> out; out.reserve(n);
	for (int i = 0; i < n; ++i) { if (i == noteOffAt) e.SetRelease(); out.push_back(e.GetValue()); }
	return out;
}

/** The original driven from explicit sample counts rather than from control
    values, so the port's widened mapping can be taken out of the comparison
    and the envelope's shape compared on its own. */
static std::vector<float> runOriginalWithCounts (long attack, long decay, float s, long release,
                                                 int n, int noteOffAt, double sr)
{
	OriginalAdsr e;
	e.SamplesPerSec = (long)sr; e.EnvEnable = true; e.EnvLevel = 1.f;
	e.SetSustainLevel(s);
	e.Attack = attack; e.Decay = decay; e.Release = release;
	std::vector<float> out; out.reserve(n);
	for (int i = 0; i < n; ++i) { if (i == noteOffAt) e.SetRelease(); out.push_back(e.GetValue()); }
	return out;
}

/** The port's sample counts for a given set of controls. */
static void portCounts (float a, float d, float r, double sr,
                        long& attack, long& decay, long& release)
{
	Adsr e; e.setSampleRate(sr);
	e.setAttackTime(a); e.setDecayTime(d); e.setReleaseTime(r);
	attack = e.attackSamples(); decay = e.decaySamples(); release = e.releaseSamples();
}

static std::vector<float> runPort (float a, float d, float s, float r,
                                   int n, int noteOffAt, double sr)
{
	Adsr e;
	e.setSampleRate(sr); e.setEnabled(true); e.setLevel(1.f);
	e.setAttackTime(a); e.setDecayTime(d); e.setSustainLevel(s); e.setReleaseTime(r);
	std::vector<float> out; out.reserve(n);
	for (int i = 0; i < n; ++i) { if (i == noteOffAt) e.noteOff(); out.push_back(e.nextValue()); }
	return out;
}

//------------------------------------------------------------------------
int main ()
{
	// 1. Given the same sample counts, the port must be bit-identical to the
	//    original at every setting a user would actually dial. Widening the
	//    range changed the counts, not the curve, and this is what says so.
	std::printf("1. port vs original at identical sample counts\n");
	int compared = 0;
	double worst = 0.0;
	for (double sr : {44100.0, 48000.0, 96000.0})
		for (float a : {0.02f, 0.2f, 0.5f, 1.0f})
			for (float d : {0.02f, 0.2f, 0.75f})
				for (float s : {0.0f, 0.5f, 0.75f, 1.0f})
					for (float r : {0.02f, 0.2f, 1.0f})
					{
						long at, dc, rl; portCounts(a,d,r, sr, at, dc, rl);
						auto o = runOriginalWithCounts(at,dc,s,rl, 8000, 5000, sr);
						auto p = runPort              (a,d,s,r,   8000, 5000, sr);
						for (size_t i = 0; i < o.size(); ++i)
							worst = std::fmax(worst, std::fabs((double)o[i] - (double)p[i]));
						++compared;
					}
	std::printf("   %d settings x 8000 samples, max deviation %g\n", compared, worst);
	chk(worst == 0.0, "the port is not bit-identical at normal settings");

	// 1b. The widened mapping must leave the FACTORY setting alone. Every
	//     time control in the parameter table defaults to 20 %, and at 20 %
	//     the squared 0..5 s / 0..10 s mapping must land on exactly the
	//     counts the DXi's linear 0..1 s / 0..2 s produced. This is the one
	//     control value where the port and the original agree by design; it
	//     is why the default patch still sounds like the DXi's.
	std::printf("1b. the factory 20%% setting still gives the DXi's times\n");
	for (double sr : {44100.0, 48000.0, 96000.0})
	{
		const float def = 0.2f;
		long at, dc, rl; portCounts(def, def, def, sr, at, dc, rl);
		OriginalAdsr o; o.SamplesPerSec = (long)sr;
		o.SetAttackTime(def); o.SetDecayTime(def); o.SetReleaseTime(def);
		std::printf("   %6.0f Hz  attack %ld/%ld  decay %ld/%ld  release %ld/%ld  (port/original)\n",
		            sr, at, o.Attack, dc, o.Decay, rl, o.Release);
		chk(at == o.Attack,  "the default attack time moved");
		chk(dc == o.Decay,   "the default decay time moved");
		chk(rl == o.Release, "the default release time moved");
	}

	// 1c. The top of the travel reaches the widened maxima, and the 1:2
	//     attack-to-release asymmetry survives at every setting.
	std::printf("1c. the widened range\n");
	{
		const double sr = 48000.0;
		long at, dc, rl; portCounts(1.f, 1.f, 1.f, sr, at, dc, rl);
		std::printf("   full travel: attack %.3f s  decay %.3f s  release %.3f s\n",
		            at/sr, dc/sr, rl/sr);
		chk(std::fabs(at/sr - Adsr::kMaxAttackDecaySeconds) < 1e-3, "attack does not reach its maximum");
		chk(std::fabs(dc/sr - Adsr::kMaxAttackDecaySeconds) < 1e-3, "decay does not reach its maximum");
		chk(std::fabs(rl/sr - Adsr::kMaxReleaseSeconds)     < 1e-3, "release does not reach its maximum");
		bool ratioHolds = true;
		for (float c : {0.1f, 0.2f, 0.35f, 0.5f, 0.8f, 1.0f})
		{
			long a2, d2, r2; portCounts(c, c, c, sr, a2, d2, r2);
			if (std::labs(r2 - 2*a2) > 2) ratioHolds = false;
		}
		chk(ratioHolds, "release is no longer exactly twice attack");
	}

	// 2. Attack at zero is reachable from the panel, and it broke the original.
	std::printf("2. attack = 0, decay = 0 - reachable, both controls bottom out\n");
	auto bad  = runOriginal(0.f, 0.f, 0.5f, 0.2f, 64, -1, 48000.0);
	auto good = runPort    (0.f, 0.f, 0.5f, 0.2f, 64, -1, 48000.0);
	int nanOrInf = 0;
	for (float v : bad) if (!std::isfinite(v)) ++nanOrInf;
	std::printf("   original: %d of %d samples non-finite (first = %f)\n",
	            nanOrInf, (int)bad.size(), bad[0]);
	chk(nanOrInf > 0, "expected the original to break here - check the transcription");
	int portBad = 0; float lo = 1e9f, hi = -1e9f;
	for (float v : good) { if (!std::isfinite(v)) ++portBad; lo = std::fmin(lo,v); hi = std::fmax(hi,v); }
	std::printf("   port:     %d non-finite, range [%.4f, %.4f]\n", portBad, lo, hi);
	chk(portBad == 0, "the port produces non-finite output at attack = 0");
	chk(lo >= 0.f, "the port holds a NEGATIVE envelope at attack = 0 - a phase inversion");
	chk(hi <= 1.05f, "the port overshoots badly at attack = 0");

	// 2b. The one-sample overshoot past 1.0 is the ORIGINAL's behaviour, not
	//     something the port introduced, so it is not clamped away.
	{
		auto o = runOriginal(0.2f, 0.2f, 0.75f, 0.2f, 40000, -1, 48000.0);
		float peak = 0.f; for (float v : o) peak = std::fmax(peak, v);
		std::printf("   original at a normal setting peaks at %.6f (>1 by one sample)\n", peak);
		chk(peak > 1.f, "expected the original to overshoot 1.0 - check the transcription");
	}

	// 3. The sustain quirk: the original never assigns the sustain level, so
	//    the note holds at or just below it. Preserved deliberately.
	std::printf("3. the sustain level is approached from above, never assigned\n");
	auto sus = runPort(0.05f, 0.05f, 0.75f, 0.2f, 20000, -1, 48000.0);
	float held = sus[15000];
	std::printf("   sustain control 0.75, value held = %.6f\n", held);
	chk(held <= 0.75f && held > 0.74f, "sustain is not holding just below its level");

	// 4. Release starts from wherever the envelope actually was.
	std::printf("4. release starts from the current value, not from sustain\n");
	auto early = runPort(0.5f, 0.2f, 0.75f, 0.2f, 8000, 4000, 48000.0);  // off mid-attack
	chk(early[4001] < 0.95f && early[4001] > 0.0f, "release did not start mid-attack");
	bool monotonic = true;
	for (size_t i = 4002; i < early.size(); ++i)
		if (early[i] > early[i-1] + 1e-6f) monotonic = false;
	chk(monotonic, "release is not monotonically falling");

	// 5. A disabled envelope is unity, and still advances - as the original.
	std::printf("5. disabled envelope returns unity\n");
	{
		Adsr e; e.setSampleRate(48000.0); e.setEnabled(false);
		e.setAttackTime(0.2f); e.setDecayTime(0.2f); e.setSustainLevel(0.75f); e.setReleaseTime(0.2f);
		bool allOne = true;
		for (int i = 0; i < 1000; ++i) if (e.nextValue() != 1.f) allOne = false;
		chk(allOne, "disabled envelope does not return 1.0");
	}

	// 6. traceEnvelope - what the editor's three envelope panels draw. The
	//    point of driving the real class is that the picture inherits the
	//    quirks, so that is what is checked.
	std::printf("6. traceEnvelope\n");
	{
		constexpr int N = 200;
		float buf[N];

		// 6a. Shape: rises to a peak, falls to sustain, holds, falls to zero.
		{
			EnvelopeSpec spec;                       // factory: A20 D20 S75 R20
			const double span = traceEnvelope(spec, buf, N);
			int peakAt = 0;
			for (int i = 1; i < N; ++i) if (buf[i] > buf[peakAt]) peakAt = i;
			std::printf("   factory spec spans %.3f s, peak %.4f at point %d, ends %.4f\n",
			            span, buf[peakAt], peakAt, buf[N-1]);
			chk(span > 0.0, "the factory envelope reports no duration");
			chk(buf[0] < 0.1f, "the trace does not start near zero");
			chk(peakAt > 0 && peakAt < N/2, "the peak is not early in the trace");
			chk(buf[peakAt] > 0.9f, "the envelope never reaches full level");
			chk(buf[N-1] < 0.05f, "the trace does not end at zero");
			bool sawSustain = false;
			for (int i = peakAt; i < N; ++i)
				if (std::fabs(buf[i] - 0.75f) < 0.03f) sawSustain = true;
			chk(sawSustain, "the sustain level never appears in the trace");
		}

		// 6b. The duration must match the controls. Attack and decay are 5 s
		//     at full travel, release 10 s, and the hold is a quarter of
		//     attack+decay - so a fully-open envelope spans about 21 s.
		{
			EnvelopeSpec spec; spec.attack=1.f; spec.decay=1.f; spec.sustain=0.f; spec.release=1.f;
			const double span = traceEnvelope(spec, buf, N);
			std::printf("   everything at maximum spans %.2f s\n", span);
			// attack 5 + decay 5 + hold 2.5 + release 0 (sustain is already 0)
			chk(span > 12.0 && span < 13.0, "a fully open envelope does not span ~12.5 s");
		}
		{
			EnvelopeSpec spec; spec.attack=1.f; spec.decay=1.f; spec.sustain=1.f; spec.release=1.f;
			const double span = traceEnvelope(spec, buf, N);
			std::printf("   maximum with sustain 100%% spans %.2f s\n", span);
			// attack 5 + decay 0 + hold 1.25 + release 10
			chk(span > 16.0 && span < 17.0, "sustain 100% does not give the longest tail");
		}

		// 6c. Level scales the drawing, so the panel shows it.
		{
			EnvelopeSpec spec; spec.level = 0.5f;
			traceEnvelope(spec, buf, N);
			float peak = 0.f; for (float v : buf) peak = std::fmax(peak, v);
			std::printf("   level 50%% peaks at %.4f\n", peak);
			chk(peak > 0.45f && peak < 0.56f, "level does not scale the trace");
		}

		// 6d. Invert turns it upside down - starts high, ends high.
		{
			EnvelopeSpec spec; spec.invert = true;
			traceEnvelope(spec, buf, N);
			std::printf("   inverted: starts %.3f, mid %.3f, ends %.3f\n",
			            buf[0], buf[N/2], buf[N-1]);
			chk(buf[0] > 0.9f, "an inverted envelope does not start at full");
			chk(buf[N-1] > 0.9f, "an inverted envelope does not end at full");
		}

		// 6e. A disabled envelope is a flat 1.0 with no duration - which is
		//     exactly what nextValue returns for one, level and invert
		//     ignored. Drawing anything else would misreport the sound.
		{
			EnvelopeSpec spec; spec.enabled = false; spec.level = 0.25f; spec.invert = true;
			const double span = traceEnvelope(spec, buf, N);
			bool flat = true;
			for (float v : buf) if (v != 1.f) flat = false;
			std::printf("   disabled: flat=%s span=%.1f\n", flat ? "yes" : "no", span);
			chk(flat, "a disabled envelope does not draw as a flat 1.0");
			chk(span == 0.0, "a disabled envelope reports a duration");
		}

		// 6f. Every reachable setting must terminate, stay finite and stay in
		//     range. This runs on the host's UI thread, so a runaway loop
		//     here freezes the host.
		{
			int bad = 0, cases = 0; double longest = 0.0;
			for (float a : {0.f, 0.02f, 0.5f, 1.f})
				for (float d : {0.f, 0.02f, 0.5f, 1.f})
					for (float sus : {0.f, 0.5f, 1.f})
						for (float r : {0.f, 0.02f, 1.f})
							for (float lv : {0.f, 1.f})
							{
								EnvelopeSpec spec;
								spec.attack=a; spec.decay=d; spec.sustain=sus;
								spec.release=r; spec.level=lv;
								const double span = traceEnvelope(spec, buf, N);
								longest = std::fmax(longest, span);
								++cases;
								for (float v : buf)
									if (!std::isfinite(v) || v < -0.01f || v > 1.10f) ++bad;
							}
			std::printf("   %d specs traced, %d bad points, longest %.2f s\n",
			            cases, bad, longest);
			chk(bad == 0, "traceEnvelope produced a non-finite or out-of-range point");
			chk(longest < 40.0, "a trace ran to the safety cap - it would hang the UI");
		}

		// 6g. The trace must respond to the controls: two different decays
		//     must not draw the same picture.
		{
			float slow[N];
			EnvelopeSpec fast; fast.decay = 0.1f;
			EnvelopeSpec slower = fast; slower.decay = 0.9f;
			const double a = traceEnvelope(fast, buf, N);
			const double b = traceEnvelope(slower, slow, N);
			double diff = 0.0;
			for (int i = 0; i < N; ++i) diff += std::fabs(buf[i] - slow[i]);
			std::printf("   decay 10%% spans %.3f s, 90%% spans %.3f s, curves differ by %.2f\n",
			            a, b, diff);
			chk(b > a, "a longer decay does not lengthen the trace");
			chk(diff > 1.0, "changing decay does not change the drawn curve");
		}
	}

	std::printf(fails ? "\n%d CHECK(S) FAILED\n" : "\nall checks passed\n", fails);
	return fails != 0;
}
