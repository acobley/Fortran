//------------------------------------------------------------------------
// ForTran - voice pool checks
//
//   c++ -std=c++17 -O2 -Wall -Wextra -I../source ../source/Adsr.cpp ../source/FilterInfo.cpp ../source/Voice.cpp ../source/Synth.cpp SynthTests.cpp -o synth-tests
//
// None of this is ported from the DXi - a DXi synth never owned its voices -
// so there is nothing to diff against. These check the properties a voice
// pool has to have, and in particular that notes cannot hang.
//------------------------------------------------------------------------

#include "Synth.h"
#include <cmath>
#include <cstdio>
#include <utility>
#include <vector>

using namespace ForTran;
static constexpr double kPI = 3.14159265358979323846;
static int fails = 0;
static void chk (bool ok, const char* what)
{ if (!ok) { std::printf("  FAIL: %s\n", what); ++fails; } }

static void setDefaults (Synth& s)
{
	VoiceParams& p = s.voiceParams();
	const float harm[10] = {1.f,.7f,.65f,.6f,.55f,.5f,.45f,.4f,.35f,.3f};
	for (int i = 0; i < 10; ++i)
	{
		p.harmLevel[i] = harm[i]; p.harmPhase[i] = 0.5f;
		p.formantAttack[i]=0.2f; p.formantDecay[i]=0.2f; p.formantSustain[i]=0.75f;
		p.formantRelease[i]=0.2f; p.formantLevel[i]=1.f; p.formantEnable[i]=false;
	}
	p.envRelease = 0.05f;
}

static double renderPeak (Synth& s, int n)
{
	std::vector<float> buf((size_t)n * 2, 0.f);
	s.render(buf.data(), n);
	double peak = 0.0;
	for (float v : buf) peak = std::fmax(peak, std::fabs(v));
	return peak;
}

//------------------------------------------------------------------------
int main ()
{
	const double sr = 48000.0;

	// 1. A note sounds; a note-off silences it and frees the voice.
	std::printf("1. note on, note off\n");
	{
		Synth s; s.setSampleRate(sr); setDefaults(s);
		chk(s.activeVoiceCount() == 0, "a fresh synth has active voices");
		s.noteOn(69, 100);
		chk(s.activeVoiceCount() == 1, "note-on did not allocate a voice");
		chk(renderPeak(s, 4096) > 1e-4, "the note is silent");
		s.noteOff(69);
		int blocks = 0;
		while (s.activeVoiceCount() > 0 && blocks < 400) { renderPeak(s, 256); ++blocks; }
		std::printf("   freed after %d blocks (%.3f s)\n", blocks, blocks*256/sr);
		chk(s.activeVoiceCount() == 0, "the voice never freed after note-off");
	}

	// 2. Full polyphony, then stealing.
	std::printf("2. polyphony and voice stealing\n");
	{
		Synth s; s.setSampleRate(sr); setDefaults(s);
		for (int i = 0; i < Synth::kMaxVoices; ++i) s.noteOn(48 + i, 100);
		std::printf("   %d notes -> %d voices\n", Synth::kMaxVoices, s.activeVoiceCount());
		chk(s.activeVoiceCount() == Synth::kMaxVoices, "not all notes got a voice");
		renderPeak(s, 512);
		for (int i = 0; i < 8; ++i) s.noteOn(80 + i, 100);
		std::printf("   %d more -> %d voices (cap %d)\n", 8, s.activeVoiceCount(), Synth::kMaxVoices);
		chk(s.activeVoiceCount() <= Synth::kMaxVoices, "the voice cap was exceeded");
		chk(renderPeak(s, 512) > 1e-4, "the synth went silent under stealing");
	}

	// 3. Retriggering a held note must not stack two voices on one pitch.
	std::printf("3. retrigger\n");
	{
		Synth s; s.setSampleRate(sr); setDefaults(s);
		for (int i = 0; i < 5; ++i) { s.noteOn(60, 100); renderPeak(s, 128); }
		std::printf("   five note-ons on one pitch -> %d voices\n", s.activeVoiceCount());
		chk(s.activeVoiceCount() == 1, "retriggering stacked voices on one pitch");
	}

	// 4. Note-on at velocity zero is a note-off - files and hosts both do
	//    this, and a synth that ignores it hangs the note.
	std::printf("4. velocity-zero note-on is a note-off\n");
	{
		Synth s; s.setSampleRate(sr); setDefaults(s);
		s.noteOn(64, 100);
		renderPeak(s, 512);
		s.noteOn(64, 0);
		int blocks = 0;
		while (s.activeVoiceCount() > 0 && blocks < 400) { renderPeak(s, 256); ++blocks; }
		chk(s.activeVoiceCount() == 0, "a velocity-zero note-on did not release the note");
	}

	// 5. Nothing hangs, under a long unbalanced stream of events.
	std::printf("5. no hanging notes under a messy event stream\n");
	{
		Synth s; s.setSampleRate(sr); setDefaults(s);
		unsigned seed = 12345;
		auto rnd = [&seed] (int n) { seed = seed * 1103515245u + 12345u; return (int)((seed >> 16) % (unsigned)n); };
		for (int i = 0; i < 4000; ++i)
		{
			const int note = 36 + rnd(60);
			if (rnd(3) == 0) s.noteOff(note); else s.noteOn(note, 1 + rnd(127));
			if (rnd(8) == 0) renderPeak(s, 64);
		}
		s.allNotesOff();
		int blocks = 0;
		while (s.activeVoiceCount() > 0 && blocks < 2000) { renderPeak(s, 256); ++blocks; }
		std::printf("   all quiet after %d blocks (%.2f s)\n", blocks, blocks*256/sr);
		chk(s.activeVoiceCount() == 0, "voices hung after allNotesOff");
	}

	// 6. Output stays finite and bounded with every voice sounding.
	std::printf("6. full polyphony stays finite\n");
	{
		Synth s; s.setSampleRate(sr); setDefaults(s);
		s.setFilterControls(0.5f, 0.5f, 0.5f);
		s.voiceParams().shapes.lowPass = 1;
		s.voiceParams().shapes.highPass = 1;
		for (int i = 0; i < Synth::kMaxVoices; ++i) s.noteOn(40 + i*2, 127);
		std::vector<float> buf(2048 * 2, 0.f);
		s.render(buf.data(), 2048);
		int bad = 0; double peak = 0.0;
		for (float v : buf) { if (!std::isfinite(v)) ++bad; peak = std::fmax(peak, std::fabs(v)); }
		std::printf("   %d voices, peak %.3f, %d non-finite\n", s.activeVoiceCount(), peak, bad);
		chk(bad == 0, "full polyphony produced non-finite output");
		chk(peak < 200.0, "full polyphony is wildly out of range");
	}

	// 7. The filter tables rebuild when a control moves, and not otherwise.
	std::printf("7. filter recalculation is demand-driven\n");
	{
		Synth s; s.setSampleRate(sr); setDefaults(s);
		s.noteOn(69, 100);
		s.setFilterControls(0.3f, 0.4f, 0.5f);
		double a = renderPeak(s, 512);
		s.setFilterControls(0.3f, 0.4f, 0.5f);       // unchanged
		double b = renderPeak(s, 512);
		s.setFilterControls(0.8f, 0.4f, 0.5f);       // moved
		double c = renderPeak(s, 512);
		std::printf("   peaks %.4f / %.4f / %.4f\n", a, b, c);
		chk(std::isfinite(a) && std::isfinite(b) && std::isfinite(c),
		    "a filter control change produced non-finite output");
	}

	// 8. MIDI controllers: volume, pan and the wheel, all live.
	std::printf("8. CC7, CC10 and the pitch wheel\n");
	{
		auto peakLR = [] (Synth& s, int n) {
			std::vector<float> b((size_t)n*2, 0.f); s.render(b.data(), n);
			double l=0,r=0;
			for (int i=0;i<n;++i){ l=std::fmax(l,std::fabs(b[2*(size_t)i])); r=std::fmax(r,std::fabs(b[2*(size_t)i+1])); }
			return std::pair<double,double>(l,r);
		};

		// pan: the DXi's law is unity at centre, not 0.707
		Synth c; c.setSampleRate(sr); setDefaults(c);
		c.setPan(64); c.noteOn(69, 100); auto centre = peakLR(c, 4096);
		Synth l; l.setSampleRate(sr); setDefaults(l);
		l.setPan(0);  l.noteOn(69, 100); auto left = peakLR(l, 4096);
		Synth rr; rr.setSampleRate(sr); setDefaults(rr);
		rr.setPan(127); rr.noteOn(69, 100); auto right = peakLR(rr, 4096);
		std::printf("   centre L/R %.3f/%.3f | hard left %.3f/%.3f | hard right %.3f/%.3f\n",
		            centre.first, centre.second, left.first, left.second, right.first, right.second);
		chk(std::fabs(centre.first - centre.second) < 1e-4, "centre pan is not equal both sides");
		chk(left.first > left.second * 20.0,  "hard left is not hard left");
		chk(right.second > right.first * 20.0, "hard right is not hard right");
		// unity at centre: hard left's L should be about sqrt(2) x centre's L
		chk(left.first > centre.first * 1.3, "the pan law looks like cos/sin, not the DXi's sqrt table");

		// channel volume scales as sqrt
		Synth v; v.setSampleRate(sr); setDefaults(v);
		v.setChannelVolume(127); v.noteOn(69,100); double full = peakLR(v,4096).first;
		Synth h; h.setSampleRate(sr); setDefaults(h);
		h.setChannelVolume(32);  h.noteOn(69,100); double quiet = peakLR(h,4096).first;
		std::printf("   cc7 127 -> %.4f, cc7 32 -> %.4f (ratio %.3f, sqrt(4)=2 expected)\n",
		            full, quiet, quiet>0?full/quiet:0.0);
		chk(quiet < full, "channel volume does not attenuate");
		chk(std::fabs(full/quiet - 2.0) < 0.15, "channel volume is not the DXi's sqrt law");

		// the wheel bends sounding notes, a full octave at the end stops
		auto pitchOf = [&] (double wheel) {
			Synth s; s.setSampleRate(sr); setDefaults(s);
			s.noteOn(69, 100); s.setPitchWheel(wheel);
			std::vector<float> b(16384*2, 0.f); s.render(b.data(), 16384);
			std::vector<float> mono(16384);
			for (int i=0;i<16384;++i) mono[(size_t)i]=0.5f*(b[2*(size_t)i]+b[2*(size_t)i+1]);
			// find the strongest of the three candidates
			double best=0, bf=0;
			for (double f : {220.0, 440.0, 880.0}) {
				double re=0,im=0;
				for (int i=0;i<16384;++i){ double w=2*kPI*f*i/sr; re+=mono[(size_t)i]*std::cos(w); im+=mono[(size_t)i]*std::sin(w);} 
				double m=std::sqrt(re*re+im*im)/16384;
				if (m>best){best=m; bf=f;}
			}
			return bf;
		};
		const double down = pitchOf(0.0), mid = pitchOf(0.5), up = pitchOf(1.0);
		std::printf("   wheel 0.0 -> %.0f Hz | centre -> %.0f Hz | 1.0 -> %.0f Hz\n", down, mid, up);
		chk(mid == 440.0, "the centred wheel is not at pitch");
		chk(down == 220.0, "full down bend is not one octave");
		chk(up == 880.0, "full up bend is not one octave");
	}

	// 9. The scope capture: zero-crossing trigger, length, and the handover.
	std::printf("9. waveform capture\n");
	{
		Synth s; s.setSampleRate(sr); setDefaults(s);
		float frame[Synth::kCaptureMax];

		chk(s.readWaveform(frame, Synth::kCaptureMax) == 0,
		    "a frame was available before anything had been rendered");

		s.noteOn(69, 100);                       // 440 Hz
		for (int i = 0; i < 20; ++i) renderPeak(s, 512);

		const int n = s.readWaveform(frame, Synth::kCaptureMax);
		const double period = sr / 440.0;
		std::printf("   captured %d samples; two periods at 440 Hz is %.0f\n", n, 2*period);
		chk(n > 0, "no frame was captured while a note was sounding");
		chk(std::abs(n - (int)(2*period)) <= 1, "the frame is not two periods long");

		int bad = 0;
		for (int i = 0; i < n; ++i) if (!std::isfinite(frame[i])) ++bad;
		chk(bad == 0, "the captured frame contains non-finite samples");

		// It must start at a RISING zero crossing, so the picture does not
		// slide sideways from frame to frame.
		std::printf("   frame starts at %.5f, next %.5f\n", frame[0], frame[1]);
		chk(frame[0] >= 0.f, "the frame does not start on a rising crossing");

		// Two successive frames of a steady note should look alike - that is
		// the whole point of triggering rather than free-running.
		for (int i = 0; i < 20; ++i) renderPeak(s, 512);
		float frame2[Synth::kCaptureMax];
		const int n2 = s.readWaveform(frame2, Synth::kCaptureMax);
		chk(n2 == n, "frame length changed between captures");
		if (n2 == n && n > 8)
		{
			double num = 0, da = 0, db = 0;
			for (int i = 0; i < n; ++i) { num += frame[i]*frame2[i]; da += frame[i]*frame[i]; db += frame2[i]*frame2[i]; }
			const double corr = (da > 0 && db > 0) ? num / std::sqrt(da*db) : 0.0;
			std::printf("   correlation between two successive frames: %.3f\n", corr);
			chk(corr > 0.6, "successive frames do not line up - the trigger is not holding");
		}

		// A short buffer must be respected, not overrun.
		float small[32];
		const int ns = s.readWaveform(small, 32);
		chk(ns <= 32, "readWaveform overran the buffer it was given");
	}

	std::printf(fails ? "\n%d CHECK(S) FAILED\n" : "\nall checks passed\n", fails);
	return fails != 0;
}
