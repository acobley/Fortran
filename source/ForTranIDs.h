//------------------------------------------------------------------------
// ForTran - class UIDs and message identifiers
//
// These UIDs are freshly generated for this port. NEVER CHANGE THEM once a
// build has been shipped: hosts store them in the project file, so a changed
// UID means every existing session silently loses the plug-in.
//------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace ForTran {

static const Steinberg::FUID kForTranProcessorUID  (0x3B4723D5, 0xF76D4616, 0xAA9D9320, 0x43C2D59E);
static const Steinberg::FUID kForTranControllerUID (0x29D6D36A, 0xB7EC46DB, 0xB03377F9, 0x8A8D647D);

// The plug-in category is declared once, in ForTranEntry.cpp, using the SDK's
// own PlugType::kInstrumentSynth - not repeated as a string here.

/** Processor -> controller. Sent from setActive, which VST3 documents as
    [UI-thread]: a message sent from process() is silently discarded by the
    host's connection proxy - see PORTING-GUIDE section 7. */
static const char* const kForTranSampleRateMessage   = "ForTranSampleRate";
static const char* const kForTranSampleRateAttribute = "SampleRate";

/** The editor's scope. The controller asks for a frame on a timer and the
    processor answers with one; both directions travel on the UI thread, so
    both are legal - unlike a message sent from process(), which the host's
    connection proxy discards. See PORTING-GUIDE section 7. */
static const char* const kForTranWaveRequestMessage = "ForTranWaveRequest";
static const char* const kForTranWaveDataMessage    = "ForTranWaveData";
static const char* const kForTranWaveAttribute      = "Wave";
/** Sent with every reply, frame or no frame: how many voices are sounding,
    for the panel's voice meter. */
static const char* const kForTranVoicesAttribute    = "Voices";

//------------------------------------------------------------------------
} // namespace ForTran
