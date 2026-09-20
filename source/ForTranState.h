//------------------------------------------------------------------------
// ForTran - the component state, written and read in one place
//
// The processor saves it, a saved project restores it, a .vstpreset carries
// it as its Comp chunk, and tools/make-presets.cpp writes one without a
// processor existing at all. Four callers, so the format lives here rather
// than being spelled out wherever it is needed: a preset writer that drifted
// from the reader would produce files that load silently wrong, which is the
// worst kind of broken.
//
// Version 1:
//     int32   stream version (1)
//     int32   parameter count
//     double  x count, normalised, indexed by ParamID
//     int32   bypass
//
// The count is in the stream, so a state written before a parameter existed
// still loads - see readComponentState, which defaults the rest.
//------------------------------------------------------------------------

#pragma once

#include "ForTranParams.h"

#include "pluginterfaces/base/ibstream.h"

namespace ForTran {

/** The version this build writes. */
constexpr Steinberg::int32 kStateVersion = 1;

/** Write `kNumParams` normalised values and the bypass flag. */
bool writeComponentState (Steinberg::IBStream* state, const double* params, bool bypass);

/** Read them back. `params` must hold kNumParams doubles; anything the
    stream does not mention is set to that parameter's DEFAULT rather than
    left as it was, so loading an old project after a new one cannot inherit
    the new one's settings. Returns false only if the stream is unreadable. */
bool readComponentState (Steinberg::IBStream* state, double* params, bool& bypass);

//------------------------------------------------------------------------
} // namespace ForTran
