//------------------------------------------------------------------------
// ForTran - plug-in factory (was PlugInApp.cpp)
//------------------------------------------------------------------------

#include "ForTranController.h"
#include "ForTranIDs.h"
#include "ForTranProcessor.h"
#include "version.h"

#include "public.sdk/source/main/pluginfactory.h"

#define stringPluginName "ForTran"

using namespace Steinberg::Vst;
using namespace ForTran;

//------------------------------------------------------------------------
BEGIN_FACTORY_DEF (stringCompanyName, "https://github.com/", "mailto:aecobley@googlemail.com")

	// ForTran is an instrument, not an effect: PlugType::kInstrumentSynth.
	// This string is what a host reads to decide which list to put it in.
	DEF_CLASS2 (INLINE_UID_FROM_FUID (kForTranProcessorUID),
	            PClassInfo::kManyInstances,
	            kVstAudioEffectClass,
	            stringPluginName,
	            Vst::kDistributable,
	            PlugType::kInstrumentSynth,
	            FULL_VERSION_STR,
	            kVstVersionString,
	            ForTranProcessor::createInstance)

	DEF_CLASS2 (INLINE_UID_FROM_FUID (kForTranControllerUID),
	            PClassInfo::kManyInstances,
	            kVstComponentControllerClass,
	            stringPluginName "Controller",
	            0,
	            "",
	            FULL_VERSION_STR,
	            kVstVersionString,
	            ForTranController::createInstance)

END_FACTORY
