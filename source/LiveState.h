//------------------------------------------------------------------------
// ForTran - what the voices are doing right now, for the panel
//
// The DSP publishes one of these per block (Synth::render) and the editor
// asks for it 30 times a second, riding the same message as the scope
// frame. It describes ONE voice - the most recently started one still
// sounding - because a panel can only animate one envelope at a time, and
// the newest note is the one being listened to.
//
// Plain data, no SDK: it crosses the processor/controller boundary as the
// bytes of a binary message attribute, which is only sound because both
// sides are built from this one header in the same binary.
//------------------------------------------------------------------------

#pragma once

namespace ForTran {

struct LiveState
{
	int   voices = 0;            // sounding voices, 0..16
	int   note = -1;             // MIDI note of the tracked voice; -1 none
	float elapsed = 0.f;         // seconds since its note-on
	float sinceRelease = -1.f;   // seconds since its note-off; -1 while held
	float ampEnv = 0.f;          // amplitude envelope, as the voice applied it
	float filterEnv = 0.f;       // filter envelope, 0..level
	float filterBin = 276.f;     // where the sweep is reading the filter, fractional
	float formantEnv[10] = {};   // the ten partial envelopes
	float partialAmp[10] = {};   // |LFO x envelope x level x filter| per partial
	float lfoPhase[10] = {};     // harmonic LFO phases, radians 0..2pi
};

} // namespace ForTran
