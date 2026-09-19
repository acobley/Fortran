//------------------------------------------------------------------------
// ForTran - voice checks
//
//   c++ -std=c++17 -O2 -Wall -Wextra -I../source ../source/Adsr.cpp ../source/FilterInfo.cpp ../source/Voice.cpp VoiceTests.cpp -o voice-tests
//
// The whole voice is too large to diff against a transcription the way the
// ADSR and the filter tables were, so these check the properties that must
// hold instead: the partials land where they should, the degenerate control
// settings that produced infinities in the DXi are finite here, and a
// released note actually stops.
//------------------------------------------------------------------------

#include "Adsr.h"
#include "Voice.h"
#include "FilterInfo.h"
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <vector>

using namespace ForTran;
static int fails = 0;
static void chk (bool ok, const char* what)
{ if (!ok) { std::printf("  FAIL: %s\n", what); ++fails; } }

static constexpr double kPI = 3.14159265358979323846;

//------------------------------------------------------------------------
static VoiceParams defaultParams ()
{
	VoiceParams p;
	const float harm[10] = {1.f, .7f, .65f, .6f, .55f, .5f, .45f, .4f, .35f, .3f};
	for (int i = 0; i < 10; ++i)
	{
		p.harmLevel[i]      = harm[i];
		p.harmPhase[i]      = 0.5f;
		p.harmInvert[i]     = false;
		p.formantAttack[i]  = 0.2f;
		p.formantDecay[i]   = 0.2f;
		p.formantSustain[i] = 0.75f;
		p.formantRelease[i] = 0.2f;
		p.formantLevel[i]   = 1.f;
		p.formantEnable[i]  = false;   // the DXi default: envelope off = unity
		p.formantInvert[i]  = false;
	}
	return p;
}

/** Magnitude of one frequency, by direct correlation. */
static double bandMagnitude (const std::vector<float>& mono, double freq, double sr)
{
	double re = 0.0, im = 0.0;
	const int n = (int)mono.size();
	for (int i = 0; i < n; ++i)
	{
		const double w = 2.0 * kPI * freq * i / sr;
		re += mono[i] * std::cos(w);
		im += mono[i] * std::sin(w);
	}
	return std::sqrt(re*re + im*im) / n;
}

static std::vector<float> renderMono (Voice& v, const FilterInfo& fi, int n)
{
	std::vector<float> st((size_t)n * 2, 0.f);
	v.render(st.data(), n, fi);
	std::vector<float> mono((size_t)n);
	for (int i = 0; i < n; ++i) mono[(size_t)i] = 0.5f * (st[2*(size_t)i] + st[2*(size_t)i+1]);
	return mono;
}

//------------------------------------------------------------------------
int main ()
{
	FilterInfo fi;
	const double sr = 48000.0;

	// 1. A plain note sounds, and lands on its own fundamental.
	std::printf("1. a note at 440 Hz, all filter shapes off\n");
	{
		Voice v;
		VoiceParams p = defaultParams();
		v.noteOn(69, 440.0, 100, 127, 64, p, sr);
		auto mono = renderMono(v, fi, 16384);

		double peak = 0.0;
		for (float s : mono) peak = std::fmax(peak, std::fabs(s));
		std::printf("   peak %.6f\n", peak);
		chk(peak > 1e-4, "the voice is silent");
		chk(peak < 4.0,  "the voice is absurdly loud");

		const double f0 = bandMagnitude(mono, 440.0, sr);
		const double f2 = bandMagnitude(mono, 880.0, sr);
		const double off = bandMagnitude(mono, 660.0, sr);   // not a harmonic
		std::printf("   440 Hz %.6f | 880 Hz %.6f | 660 Hz (not a harmonic) %.6f\n", f0, f2, off);
		chk(f0 > 10.0 * off, "no energy at the fundamental");
		chk(f2 > 10.0 * off, "no energy at the second harmonic");
	}

	// 2. Harmonic levels do what they say.
	std::printf("2. silencing harmonic 2 removes 880 Hz\n");
	{
		VoiceParams p = defaultParams();
		Voice a; a.noteOn(69, 440.0, 100, 127, 64, p, sr);
		double with = bandMagnitude(renderMono(a, fi, 16384), 880.0, sr);

		p.harmLevel[1] = 0.f;
		Voice b; b.noteOn(69, 440.0, 100, 127, 64, p, sr);
		double without = bandMagnitude(renderMono(b, fi, 16384), 880.0, sr);

		std::printf("   harmonic 2 on %.6f -> off %.6f\n", with, without);
		chk(without < with * 0.1, "silencing harmonic 2 did not remove 880 Hz");
	}

	// 3. The control settings that produced infinities in the DXi.
	//    Vibrato rate 0 and tremolo rate 0 divided by zero; vibrato depth 1
	//    divided by (1 - depth). All three are the bottom or top of a
	//    control the user can reach.
	std::printf("3. degenerate LFO settings that broke the original\n");
	{
		int bad = 0, cases = 0;
		for (float vr : {0.0f, 0.001f, 1.0f})
		 for (float vd : {0.0f, 0.5f, 1.0f})
		  for (float tr : {0.0f, 0.001f, 1.0f})
		   for (float td : {0.0f, 1.0f})
		   {
			   VoiceParams p = defaultParams();
			   p.vibratoRate = vr; p.vibratoDepth = vd;
			   p.tremoloRate = tr; p.tremoloDepth = td;
			   Voice v; v.noteOn(69, 440.0, 100, 127, 64, p, sr);
			   auto mono = renderMono(v, fi, 4096);
			   for (float s : mono) if (!std::isfinite(s) || std::fabs(s) > 100.f) ++bad;
			   ++cases;
		   }
		std::printf("   %d settings x 4096 samples, %d non-finite or absurd\n", cases, bad);
		chk(bad == 0, "a degenerate LFO setting produced a bad sample");
	}

	// 4. Envelopes at the bottom of their travel - the ADSR NaN case, now
	//    reached through the whole voice rather than in isolation.
	std::printf("4. every envelope control at zero\n");
	{
		VoiceParams p = defaultParams();
		p.envAttack = p.envDecay = p.envSustain = p.envRelease = 0.f;
		p.filterAttack = p.filterDecay = p.filterSustain = p.filterRelease = 0.f;
		for (int i = 0; i < 10; ++i)
		{
			p.formantAttack[i] = p.formantDecay[i] = 0.f;
			p.formantSustain[i] = p.formantRelease[i] = 0.f;
			p.formantEnable[i] = true;
		}
		Voice v; v.noteOn(69, 440.0, 100, 127, 64, p, sr);
		auto mono = renderMono(v, fi, 8192);
		int bad = 0;
		for (float s : mono) if (!std::isfinite(s)) ++bad;
		std::printf("   %d non-finite samples\n", bad);
		chk(bad == 0, "zeroed envelopes produced non-finite output");
	}

	// 5. Every filter shape combination, through the whole voice.
	std::printf("5. all 128 shape combinations\n");
	{
		int bad = 0;
		for (int lp = 0; lp <= 1; ++lp)
		 for (int hp = 0; hp <= 3; ++hp)
		  for (int cb = 0; cb <= 3; ++cb)
		   for (int gr = 0; gr <= 3; ++gr)
		   {
			   VoiceParams p = defaultParams();
			   p.shapes.lowPass = lp; p.shapes.highPass = hp;
			   p.shapes.comb = cb;    p.shapes.grass = gr;
			   Voice v; v.noteOn(69, 440.0, 100, 127, 64, p, sr);
			   auto mono = renderMono(v, fi, 2048);
			   for (float s : mono) if (!std::isfinite(s) || std::fabs(s) > 100.f) ++bad;
		   }
		std::printf("   %d bad samples across every combination\n", bad);
		chk(bad == 0, "a shape combination produced a bad sample");
	}

	// 6. A released note stops, and stops in a bounded time.
	std::printf("6. note-off\n");
	{
		VoiceParams p = defaultParams();
		p.envRelease = 0.1f;                   // 0.1 * 2 = 0.2 s
		Voice v; v.noteOn(69, 440.0, 100, 127, 64, p, sr);
		renderMono(v, fi, 8192);
		chk(v.isActive(), "the voice went inactive while still held");
		v.noteOff();
		int blocks = 0;
		while (v.isActive() && blocks < 200) { renderMono(v, fi, 512); ++blocks; }
		const double seconds = blocks * 512 / sr;
		std::printf("   released after %d blocks (%.3f s)\n", blocks, seconds);
		chk(!v.isActive(), "the voice never went inactive after note-off");
		chk(seconds < 1.0, "the release took far longer than the control asks for");
	}

	// 7. The ladder is per-voice, so two voices sum linearly.
	std::printf("7. two voices sum linearly (per-voice ladder)\n");
	{
		VoiceParams p = defaultParams();
		Voice a, b, c, d;
		a.noteOn(60, 261.63, 100, 127, 64, p, sr);
		b.noteOn(64, 329.63, 100, 127, 64, p, sr);
		auto ma = renderMono(a, fi, 4096);
		auto mb = renderMono(b, fi, 4096);

		c.noteOn(60, 261.63, 100, 127, 64, p, sr);
		d.noteOn(64, 329.63, 100, 127, 64, p, sr);
		std::vector<float> both(4096 * 2, 0.f);
		c.render(both.data(), 4096, fi);
		d.render(both.data(), 4096, fi);

		double worst = 0.0;
		for (int i = 0; i < 4096; ++i)
		{
			const double mixed = 0.5 * (both[2*(size_t)i] + both[2*(size_t)i+1]);
			worst = std::fmax(worst, std::fabs(mixed - (ma[(size_t)i] + mb[(size_t)i])));
		}
		std::printf("   max deviation from the sum of the two rendered alone: %g\n", worst);
		chk(worst < 1e-6, "voices do not sum linearly - the ladder state is being shared");
	}

	// 8. The filter envelope's sweep must cover exactly the bins that exist.
	//    The DXi scaled it across 551 - the size of the whole table - while
	//    starting the sweep at bin 276, so the bottom half of the envelope's
	//    travel fell off the end of the table and did nothing. The invariant
	//    is that the span equals the origin; anything else silently throws
	//    part of the envelope away. See Voice.h.
	std::printf("8. the filter envelope sweeps exactly the reachable bins\n");
	{
		std::printf("   origin bin %d, sweep span %.0f bins\n",
		            Voice::kFilterBinOrigin, (double)Voice::kFilterSweepBins);
		chk((double)Voice::kFilterSweepBins == (double)Voice::kFilterBinOrigin,
		    "the sweep span and the origin disagree - part of the envelope is unreachable");

		// And the reachable end points are inside the table.
		const int atFull = Voice::kFilterBinOrigin - (int)Voice::kFilterSweepBins;
		std::printf("   envelope 0 -> bin %d (%.1f Hz), envelope 1 -> bin %d (%.1f Hz)\n",
		            Voice::kFilterBinOrigin, fi.binFrequency(Voice::kFilterBinOrigin),
		            atFull, fi.binFrequency(atFull < 0 ? 0 : atFull));
		chk(atFull >= 0, "a fully open envelope runs off the bottom of the table");
		chk(Voice::kFilterBinOrigin < FilterInfo::kNumBins, "the sweep starts off the top of the table");
	}

	// 8b. Filter Level must scale how DEEP the sweep goes. With the DXi's
	//     551 it saturated: 100 %, 75 % and 50 % all bottomed out at the
	//     same bin, so the control did nothing over its top half.
	std::printf("8b. Filter Level scales the depth of the sweep\n");
	{
		int previous = -1;
		bool strictlyDeeper = true;
		for (float level : {0.25f, 0.5f, 0.75f, 1.0f})
		{
			Adsr e; e.setSampleRate(48000.0); e.setEnabled(true); e.setLevel(level);
			e.setAttackTime(0.2f); e.setDecayTime(0.2f);
			e.setSustainLevel(0.75f); e.setReleaseTime(0.2f);
			int lowest = FilterInfo::kNumBins;
			for (long i = 0; i < 48000; ++i)
			{
				const float v = e.nextValue();
				int bin = Voice::kFilterBinOrigin - (int)(Voice::kFilterSweepBins * v);
				if (bin < 0) bin = 1;
				if (bin < lowest) lowest = bin;
			}
			std::printf("   level %3.0f%% -> deepest bin %3d (%7.1f Hz)\n",
			            level * 100.f, lowest, fi.binFrequency(lowest));
			if (previous >= 0 && lowest >= previous) strictlyDeeper = false;
			previous = lowest;
		}
		chk(strictlyDeeper, "Filter Level saturates - raising it stops deepening the sweep");
	}

	// 9. The per-harmonic LFO - the feature the DXi left unfinished.
	std::printf("9. the per-harmonic amplitude LFO\n");
	{
		const double sr = 48000.0;
		auto mono = [&](const VoiceParams& p, int n) {
			Voice v; v.noteOn(69, 440.0, 100, 127, 64, p, sr);
			std::vector<float> st((size_t)n*2, 0.f);
			v.render(st.data(), n, fi);
			std::vector<float> m((size_t)n);
			for (int i = 0; i < n; ++i) m[(size_t)i] = 0.5f*(st[2*(size_t)i]+st[2*(size_t)i+1]);
			return m;
		};

		VoiceParams base;
		for (int i = 0; i < 10; ++i) {
			base.harmLevel[i] = (i == 0) ? 1.f : 0.f;
			base.harmPhase[i] = 0.5f;
			base.harmLfoRate[i] = 0.5f;
			base.harmLfoDepth[i] = 0.f;
		}
		base.envAttack = 0.f; base.envDecay = 0.f; base.envSustain = 1.f;

		// 9a. At depth zero the LFO must not exist. Rate is then irrelevant,
		//     which is what says the term is skipped rather than computed
		//     and multiplied by one.
		{
			VoiceParams other = base;
			for (int i = 0; i < 10; ++i) other.harmLfoRate[i] = 0.95f;
			auto a = mono(base, 4096), b = mono(other, 4096);
			double worst = 0.0;
            for (size_t i = 0; i < a.size(); ++i) worst = std::fmax(worst, std::fabs((double)a[i]-b[i]));
			std::printf("   depth 0, rate 50%% vs 95%%: max deviation %g\n", worst);
			chk(worst == 0.0, "rate changes the sound at depth zero - the LFO is not being skipped");
		}

		// 9b. The two code paths must agree. A depth too small to alter any
		//     product still switches the voice onto the LFO path, so this
		//     compares the branches against each other rather than trusting
		//     that they were written the same way.
		{
			VoiceParams tiny = base;
			tiny.harmLfoDepth[0] = 1e-30f;      // active, but 1 + 1e-30*x == 1
			auto a = mono(base, 4096), b = mono(tiny, 4096);
			double worst = 0.0;
			for (size_t i = 0; i < a.size(); ++i) worst = std::fmax(worst, std::fabs((double)a[i]-b[i]));
			std::printf("   plain path vs LFO path at a null depth: max deviation %g\n", worst);
			chk(worst == 0.0, "the LFO path is not bit-identical when the depth cannot bite");
		}

		// 9c. It must actually modulate, at the rate the knob asks for. The
		//     envelope of a 440 Hz partial is measured by rectifying and
		//     counting the peaks.
		{
			VoiceParams mod = base;
			mod.harmLfoDepth[0] = 1.f;
			mod.harmLfoRate[0]  = (float)harmonicLfoControl(4.0);   // 4 Hz
			const int n = (int)(sr * 2.0);
			auto m = mono(mod, n);

			// Envelope: peak per 1 ms window, then count zero crossings of
			// the envelope about its mean.
			const int win = (int)(sr/1000.0);
			std::vector<double> env;
			for (int i = 0; i + win <= n; i += win) {
				double pk = 0.0;
				for (int j = 0; j < win; ++j) pk = std::fmax(pk, std::fabs((double)m[(size_t)(i+j)]));
				env.push_back(pk);
			}
			double mean = 0.0; for (double v : env) mean += v; mean /= (double)env.size();
			int crossings = 0;
			for (size_t i = 1; i < env.size(); ++i)
				if ((env[i-1] < mean) != (env[i] < mean)) ++crossings;
			const double hz = crossings / 2.0 / 2.0;      // crossings/2 cycles over 2 s
			std::printf("   depth 100%%, rate asked 4.00 Hz, measured %.2f Hz\n", hz);
			chk(std::fabs(hz - 4.0) < 0.6, "the LFO does not run at the rate the control asks for");

			double lo = 1e9, hi = -1e9;
			for (double v : env) { lo = std::fmin(lo,v); hi = std::fmax(hi,v); }
			std::printf("   envelope swings %.4f .. %.4f\n", lo, hi);
			chk(lo < 0.15*hi, "depth 100% does not take the partial down to near silence");
		}

		// 9d. The rate mapping, which the panel and the DSP share.
		{
			std::printf("   rate mapping: 0%% -> %.3f Hz, 50%% -> %.3f Hz, 100%% -> %.2f Hz\n",
			            harmonicLfoHz(0.0), harmonicLfoHz(0.5), harmonicLfoHz(1.0));
			chk(std::fabs(harmonicLfoHz(0.0) - kHarmLfoMinHz) < 1e-12, "the bottom of the rate knob is not the minimum");
			chk(std::fabs(harmonicLfoHz(1.0) - kHarmLfoMaxHz) < 1e-9, "the top of the rate knob is not the maximum");
			double worst = 0.0;
			for (double c = 0.0; c <= 1.0001; c += 0.01)
				worst = std::fmax(worst, std::fabs(harmonicLfoControl(harmonicLfoHz(c)) - c));
			std::printf("   rate round trip: worst error %.3g\n", worst);
			chk(worst < 1e-9, "the rate mapping does not round-trip");
			// The DXi's intended seeding: harmonic n at 1/n Hz.
			chk(std::fabs(harmonicLfoHz(0.5) - 1.0) < 1e-9, "harmonic 1's default is not 1 Hz at the knob's centre");
		}

		// 9e. Every combination of depth and rate must stay finite. Depth 1
		//     drives the multiplier to zero once a cycle; nothing downstream
		//     may divide by it.
		{
			int bad = 0;
			for (float d : {0.f, 0.5f, 1.f})
				for (float r : {0.f, 0.5f, 1.f}) {
					VoiceParams p = base;
					for (int i = 0; i < 10; ++i) { p.harmLevel[i] = 1.f; p.harmLfoDepth[i] = d; p.harmLfoRate[i] = r; }
					auto m = mono(p, 4096);
					for (float v : m) if (!std::isfinite(v) || std::fabs(v) > 100.f) ++bad;
				}
			std::printf("   9 depth x rate settings, all ten harmonics: %d bad samples\n", bad);
			chk(bad == 0, "the LFO produced a non-finite or absurd sample");
		}
	}

	// 10. The filter lookup is interpolated between bins, not truncated to
	//     one. The test is spectral, because that is what a click is: the
	//     synth is ten partials of a 440 Hz note, so it has NOTHING above
	//     4.4 kHz of its own. Any energy at 10 kHz is the sweep stepping.
	//
	//     Measured on the same note with the DXi's integer bin, the worst
	//     512-sample frame reads 0.01362; interpolated it reads 0.00014.
	//     Ninety-seven times less, about 40 dB. The threshold sits between
	//     them, an order of magnitude clear of each. See PORTING-NOTES
	//     section 6.
	std::printf("10. the filter sweep does not step\n");
	{
		const double sr = 48000.0;
		VoiceParams p;
		for (int i = 0; i < 10; ++i) { p.harmLevel[i] = 1.f - 0.07f*i; p.harmPhase[i] = 0.5f;
		                               p.formantEnable[i] = false; }
		p.shapes.lowPass = 1;
		p.filterLevel = 1.f; p.filterAttack = 0.8f; p.filterDecay = 0.8f;
		p.filterSustain = 0.f; p.filterRelease = 0.8f;
		p.envAttack = 0.1f; p.envDecay = 0.1f; p.envSustain = 1.f; p.envRelease = 0.5f;

		FilterInfo f2;
		f2.setCutoff(0.5f); f2.setFeedback(0.5f*3.9f); f2.setPoles(1.f + 0.5f*10.f);
		if (f2.needsRecalculation()) f2.calculate();

		Voice v; v.noteOn(69, 440.0, 100, 127, 64, p, sr);
		const int N = (int)(sr * 3.0);
		std::vector<float> st((size_t)N*2, 0.f);
		v.render(st.data(), N, f2);
		std::vector<double> x((size_t)N);
		double peak = 0.0;
		for (int i = 0; i < N; ++i) { x[(size_t)i] = 0.5*(st[2*(size_t)i]+st[2*(size_t)i+1]);
		                              peak = std::fmax(peak, std::fabs(x[(size_t)i])); }

		// Goertzel at 10 kHz over 512-sample frames: one number per frame,
		// and a click puts all its energy into one of them.
		const int W = 512;
		const double w = 2.0 * 3.14159265358979323846 * 10000.0 / sr;
		const double coeff = 2.0 * std::cos(w);
		double worst = 0.0; int worstAt = 0;
		for (int start = 0; start + W <= N; start += 256)
		{
			// Hann-windowed, or the note's own fundamental leaks into the
			// 10 kHz bin and swamps what is being measured.
			double s1 = 0.0, s2 = 0.0;
			for (int i = 0; i < W; ++i)
			{
				const double win = 0.5 - 0.5*std::cos(2.0*3.14159265358979323846*i/(W-1));
				const double s0 = win*x[(size_t)(start+i)] + coeff*s1 - s2;
				s2 = s1; s1 = s0;
			}
			const double mag = std::sqrt(s1*s1 + s2*s2 - coeff*s1*s2);
			if (mag > worst) { worst = mag; worstAt = start; }
		}
		std::printf("   peak %.3f, worst 10 kHz frame %.5f at %.3f s\n",
		            peak, worst, worstAt/sr);
		chk(peak > 0.5, "the test note is not sounding - the measurement means nothing");
		chk(worst < 0.002, "the sweep is putting energy above the synth's own band - it is stepping");
	}

	// 11. The ladder switch, and the measurement that made it worth having.
	//     The ladder is a fixed +10.6 dB with a corner at 0.198 x Nyquist -
	//     4.76 kHz at 48 k - and the synth has only ten partials. So for any
	//     note whose tenth partial stays under that corner, the ladder is
	//     doing nothing but the gain, and a level-matched bypass must be
	//     almost indistinguishable from it. Above the corner it must not be.
	std::printf("11. the ladder switch\n");
	{
		const double sr = 48000.0;
		FilterInfo f3;
		if (f3.needsRecalculation()) f3.calculate();

		auto renderNote = [&](double hz, bool ladder) {
			VoiceParams p;
			for (int i = 0; i < 10; ++i) { p.harmLevel[i] = 1.f - 0.07f*i;
			                               p.harmPhase[i] = 0.5f; p.formantEnable[i] = false; }
			p.envAttack = 0.f; p.envDecay = 0.f; p.envSustain = 1.f;
			Voice v; v.noteOn(60, hz, 100, 127, 64, p, sr);
			v.setLadderEnabled(ladder);
			const int n = 8192;
			std::vector<float> st((size_t)n*2, 0.f);
			v.render(st.data(), n, f3);
			std::vector<double> m((size_t)n);
			for (int i = 0; i < n; ++i) m[(size_t)i] = 0.5*(st[2*(size_t)i]+st[2*(size_t)i+1]);
			return m;
		};
		// Compared as SPECTRA, not sample by sample. Four poles delay the
		// signal even where their magnitude is flat, so two runs with
		// identical spectra still differ at every sample - a difference
		// that is a phase shift of the whole voice and inaudible on its
		// own. What "does the ladder do anything" means is what it does to
		// the partials, so that is what is measured: the worst change in
		// any of the ten, in decibels.
		auto partialsDb = [&](const std::vector<double>& x, double f0) {
			std::vector<double> mag(10);
			const size_t from = x.size()/2, n = x.size() - from;
			for (int k = 1; k <= 10; ++k)
			{
				const double w = 2.0*3.14159265358979323846*(k*f0)/sr;
				double re = 0.0, im = 0.0;
				for (size_t i = 0; i < n; ++i)
				{ re += x[from+i]*std::cos(w*i); im += x[from+i]*std::sin(w*i); }
				mag[(size_t)(k-1)] = std::sqrt(re*re + im*im) / (double)n;
			}
			return mag;
		};
		auto worstDb = [](const std::vector<double>& a, const std::vector<double>& b) {
			double worst = 0.0;
			for (size_t k = 0; k < a.size(); ++k)
			{
				if (a[k] < 1e-6 || b[k] < 1e-6) continue;
				worst = std::fmax(worst, std::fabs(20.0*std::log10(a[k]/b[k])));
			}
			return worst;
		};

		struct Case { const char* name; double hz; };
		const Case cases[] = { {"C2  65 Hz", 65.41}, {"A3 220 Hz", 220.0},
		                       {"A4 440 Hz", 440.0}, {"C6 1047 Hz", 1046.5} };
		double lowNote = -1.0, highNote = -1.0;
		for (const Case& c : cases)
		{
			auto on  = renderNote(c.hz, true);
			auto off = renderNote(c.hz, false);
			double pOn = 0.0, pOff = 0.0;
			for (size_t i = on.size()/2; i < on.size(); ++i)
			{ pOn = std::fmax(pOn, std::fabs(on[i])); pOff = std::fmax(pOff, std::fabs(off[i])); }
			const double diff = worstDb(partialsDb(on, c.hz), partialsDb(off, c.hz));
			std::printf("   %-11s peak in %.3f, bypassed %.3f, worst partial %5.2f dB\n",
			            c.name, pOn, pOff, diff);
			if (c.hz < 100.0)  lowNote  = diff;
			if (c.hz > 1000.0) highNote = diff;
		}
		chk(lowNote >= 0.0 && lowNote < 0.5,
		    "a low note is not level-matched - every partial is below the corner, so bypass should barely change it");
		chk(highNote > 6.0,
		    "a high note sounds the same either way - the switch is not switching");

		// And the switch must be live, not latched at note-on.
		{
			VoiceParams p;
			for (int i = 0; i < 10; ++i) { p.harmLevel[i] = 1.f; p.harmPhase[i] = 0.5f;
			                               p.formantEnable[i] = false; }
			p.envAttack = 0.f; p.envDecay = 0.f; p.envSustain = 1.f;
			Voice v; v.noteOn(84, 1046.5, 100, 127, 64, p, sr);
			std::vector<float> st(4096*2, 0.f);
			v.render(st.data(), 2048, f3);          // sounding, ladder in
			v.setLadderEnabled(false);
			v.render(st.data() + 2048*2, 2048, f3); // still sounding, ladder out
			double before = 0.0, after = 0.0;
			for (int i = 1024; i < 2048; ++i) before = std::fmax(before, std::fabs((double)st[2*i]));
			for (int i = 3072; i < 4096; ++i) after  = std::fmax(after,  std::fabs((double)st[2*i]));
			std::printf("   thrown mid-note at 1047 Hz: %.3f -> %.3f\n", before, after);
			chk(after > before * 1.5, "the switch does nothing to a note that is already sounding");
		}
	}

	// 12. The pan law. Its TAPER is the DXi's and was always a textbook
	//     constant-power one - L^2 + R^2 is the same at every position, so a
	//     note is equally loud wherever it sits. Only the normalisation
	//     changed: the DXi put unity at the centre, which left a hard-panned
	//     note peaking 3 dB above a centred one. Unity now sits at the
	//     extremes, the convention, so panning can never add peak level.
	std::printf("12. pan: constant power, and peak no longer rises with the pan\n");
	{
		VoiceParams p;
		for (int i = 0; i < 10; ++i) { p.harmLevel[i] = (i == 0) ? 1.f : 0.f; p.harmPhase[i] = 0.5f;
		                               p.formantEnable[i] = false; }
		p.envAttack = 0.f; p.envDecay = 0.f; p.envSustain = 1.f;

		double worstPower = 0.0, worstPeak = 0.0, centrePower = 0.0;
		for (int n = 0; n <= 127; n += 1)
		{
			Voice v; v.noteOn(69, 440.0, 100, 127, 64, p, 48000.0);
			v.setPan(n);
			std::vector<float> st(2048*2, 0.f);
			v.render(st.data(), 2048, fi);
			double pl = 0.0, pr = 0.0;
			for (int i = 1024; i < 2048; ++i)
			{ pl = std::fmax(pl, std::fabs((double)st[2*i])); pr = std::fmax(pr, std::fabs((double)st[2*i+1])); }
			const double power = pl*pl + pr*pr;
			if (n == 64) centrePower = power;
			worstPeak = std::fmax(worstPeak, std::fmax(pl, pr));
			worstPower = std::fmax(worstPower, power);
		}
		double lowestPower = 1e9;
		for (int n : {0, 32, 64, 96, 127})
		{
			Voice v; v.noteOn(69, 440.0, 100, 127, 64, p, 48000.0);
			v.setPan(n);
			std::vector<float> st(2048*2, 0.f);
			v.render(st.data(), 2048, fi);
			double pl = 0.0, pr = 0.0;
			for (int i = 1024; i < 2048; ++i)
			{ pl = std::fmax(pl, std::fabs((double)st[2*i])); pr = std::fmax(pr, std::fabs((double)st[2*i+1])); }
			std::printf("   pan %3d: L %.3f  R %.3f  power %.4f  peak %.3f\n", n, pl, pr, pl*pl+pr*pr, std::fmax(pl,pr));
			lowestPower = std::fmin(lowestPower, pl*pl + pr*pr);
		}
		// Constant power across the whole sweep - the taper is untouched.
		std::printf("   power varies by %.2f%% across all 128 positions\n",
		            100.0 * (worstPower - lowestPower) / centrePower);
		chk((worstPower - lowestPower) / centrePower < 0.02, "the pan law is no longer constant power");

		// And no position peaks above a centred, hard-panned reference: the
		// whole point of the renormalisation.
		Voice ref; ref.noteOn(69, 440.0, 100, 127, 0, p, 48000.0);
		std::vector<float> rs(2048*2, 0.f);
		ref.render(rs.data(), 2048, fi);
		double hardPeak = 0.0;
		for (int i = 1024; i < 2048; ++i)
			hardPeak = std::fmax(hardPeak, std::fabs((double)rs[2*i]));
		std::printf("   worst peak over the sweep %.3f, hard left %.3f\n", worstPeak, hardPeak);
		chk(worstPeak <= hardPeak * 1.001, "some pan position peaks above hard-panned - the renormalisation is wrong");
	}

	// -- 13. portamento ---------------------------------------------------
	//
	// Not from the DXi, which had no glide at all. Three things have to be
	// true: off is off, the glide arrives, and it is even in SEMITONES
	// rather than in hertz.
	{
		std::printf("13. portamento\n");
		const double sr = 48000.0;
		FilterInfo fi; fi.calculate();
		VoiceParams p = defaultParams();

		// (a) OFF IS THE OLD CODE. Same note, one told where to glide from
		// and one not, with the time at zero: every sample must match.
		{
			VoiceParams q = p;
			q.portamento = 0.f;
			Voice a; a.noteOn(69, 440.0, 100, 127, 64, q, sr, 220.0);
			Voice b; b.noteOn(69, 440.0, 100, 127, 64, q, sr);
			std::vector<float> sa(4096*2, 0.f), sb(4096*2, 0.f);
			a.render(sa.data(), 4096, fi);
			b.render(sb.data(), 4096, fi);
			double worst = 0.0;
			for (size_t i = 0; i < sa.size(); ++i)
				worst = std::fmax(worst, std::fabs((double)sa[i] - (double)sb[i]));
			std::printf("   portamento 0, glide source ignored: worst sample difference %g\n", worst);
            chk(worst == 0.0, "portamento zero is not bit-identical to no glide");
		}

		// (b) and (c). An octave, 220 -> 440, over a quarter of a second.
		// Measured as spectra rather than as samples: what matters is the
		// pitch at each moment, not the waveform that carries it.
		{
			VoiceParams q = p;
			q.portamento = 0.25f;
			for (int i = 1; i < 10; ++i) q.harmLevel[i] = 0.f;   // fundamental only
			Voice v; v.noteOn(57, 440.0, 100, 127, 64, q, sr, 220.0);

			const int total = (int)(0.6 * sr);
			std::vector<float> st((size_t)total * 2, 0.f);
			v.render(st.data(), total, fi);

			auto window = [&](int from, int len) {
				std::vector<float> mono((size_t)len);
				for (int i = 0; i < len; ++i) mono[(size_t)i] = st[(size_t)(from + i) * 2];
				return mono;
			};

			// At the start it is playing the note it came from.
			const std::vector<float> first = window((int)(0.005 * sr), 2048);
            const double at220 = bandMagnitude(first, 220.0, sr);
            const double at440 = bandMagnitude(first, 440.0, sr);
			std::printf("   first 43 ms:  220 Hz %.4f   440 Hz %.4f\n", at220, at440);
			chk(at220 > at440 * 4.0, "the glide does not start at the pitch it came from");

			// Half way through it is at the geometric mean - a tritone, 311 Hz
			// - and NOT at the arithmetic mean of 330, which is what a glide
			// that was linear in hertz would give.
			const std::vector<float> mid = window((int)(0.125 * sr) - 512, 1024);
			const double at311 = bandMagnitude(mid, 311.13, sr);
			const double at330 = bandMagnitude(mid, 330.0, sr);
			std::printf("   at half time: 311 Hz (even in semitones) %.4f   330 Hz (linear in Hz) %.4f\n",
			            at311, at330);
			chk(at311 > at330, "the glide is linear in hertz, not in semitones");

			// And after it, the note it was asked for.
			const std::vector<float> last = window(total - 2048, 2048);
			const double end220 = bandMagnitude(last, 220.0, sr);
			const double end440 = bandMagnitude(last, 440.0, sr);
			std::printf("   after 0.25 s: 220 Hz %.4f   440 Hz %.4f\n", end220, end440);
			chk(end440 > end220 * 4.0, "the glide does not arrive on the target pitch");
		}

		// (d) Two voices glide independently - the whole point of doing this
		// per voice rather than on one shared pitch.
		{
			VoiceParams q = p;
			q.portamento = 0.2f;
			for (int i = 1; i < 10; ++i) q.harmLevel[i] = 0.f;
			Voice a; a.noteOn(60, 261.63, 100, 127, 64, q, sr, 246.94);   // B3 -> C4
			Voice b; b.noteOn(67, 392.00, 100, 127, 64, q, sr, 415.30);   // G#4 -> G4
			const int n = 1024;
			std::vector<float> st((size_t)n * 2, 0.f);
			a.render(st.data(), n, fi);
			b.render(st.data(), n, fi);
			std::vector<float> mono((size_t)n);
			for (int i = 0; i < n; ++i) mono[(size_t)i] = st[(size_t)i * 2];
			const double up = bandMagnitude(mono, 246.94, sr);
			const double down = bandMagnitude(mono, 415.30, sr);
			std::printf("   two voices at once: 247 Hz %.4f   415 Hz %.4f (both should sound)\n", up, down);
			chk(up > 0.01 && down > 0.01, "the two voices are not gliding independently");
		}
	}

	std::printf(fails ? "\n%d CHECK(S) FAILED\n" : "\nall checks passed\n", fails);
	return fails != 0;
}
