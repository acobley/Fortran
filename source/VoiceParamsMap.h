//------------------------------------------------------------------------
// ForTran - parameters -> VoiceParams
//
// The one place that knows both the VST3 parameter table and the DSP's
// VoiceParams. Voice, Synth, Adsr and FilterInfo stay free of SDK types so
// they can be tested standalone; this file is the seam.
//
// Everything here goes through ParamDef::toInternal, so the DSP is handed
// exactly what CForTran::GetParamValue handed it.
//------------------------------------------------------------------------

#pragma once

#include "ForTranParams.h"
#include "Voice.h"

namespace ForTran {

/** Fill `out` from an array of normalised values indexed by ParamID.
    Called at note-on, latching the values for the life of the note, which
    is what the DXi's InitializeNoteEvent did. */
void buildVoiceParams (const double* normalized, VoiceParams& out);

//------------------------------------------------------------------------
} // namespace ForTran
