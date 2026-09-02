//------------------------------------------------------------------------
// ForTran - edit controller implementation
//------------------------------------------------------------------------

#include "ForTranController.h"
#include "ForTranEditor.h"
#include "ForTranIDs.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ustring.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace ForTran {

//------------------------------------------------------------------------
namespace {

/** The units string the host is told to put after the value.
 
    Normally the table's, but the 36 envelope time controls carry their own
    unit inside the value string ("200 ms", "1.25 s"), because which one it
    is depends on the value. Leaving the table's "%" here would make a host
    render "200 ms %". */
const char* unitsFor (const ParamDef& def)
{
	// Anything whose value string carries its own unit gets an empty units
	// field: the envelope times ("200 ms" / "1.25 s") and the harmonic
	// phases ("+45" plus a degree sign). Leaving the table's "%" would make
	// a host render "200 ms %".
	if (envelopeTimeRole (def.id) != TimeRole::None || isPhaseParam (def.id))
		return "";
	if (def.id == kOutputLevel)
		return "";                    // "-20.0 dB", not "-20.0 dB %"
	return def.units;
}

/** A RangeParameter whose text formatting matches the DXi property page -
    and, for the envelope times, the editor's own readout. */
class ForTranParameter : public RangeParameter
{
public:
	ForTranParameter (const ParamDef& def)
	// NOTE: title and units must not be null. RangeParameter hands both to
	// UString::assign, which dereferences without a null check - a null title
	// segfaults inside EditController::initialize, which surfaces as the SDK
	// validator dying during the post-build step. See PORTING-GUIDE section 7.
	: RangeParameter (USTRING (def.title), def.id, USTRING (unitsFor (def)),
	                  def.plainMin, def.plainMax, def.plainDefault,
	                  def.stepCount, ParameterInfo::kCanAutomate, kRootUnitId,
	                  USTRING (def.title))
	, mDef (def)
	, mTimeRole (envelopeTimeRole (def.id))
	, mIsPhase (isPhaseParam (def.id))
	{
	}

	void toString (ParamValue normalized, String128 string) const SMTG_OVERRIDE
	{
		char text[64] = {};

		// The envelope times read as times in the host too, so an automation
		// lane and the panel agree. Sustain and level are levels and fall
		// through to the percentage below.
		if (mTimeRole != TimeRole::None)
		{
			formatEnvelopeTime (envelopeTimeSeconds (mDef.id, mDef.toInternal (normalized)),
			                    text, sizeof (text));
			UString (string, str16BufferSize (String128)).assign (USTRING (text));
			return;
		}

		// The output trim reads in decibels in the host too.
		if (mDef.id == kOutputLevel)
		{
			const double inner = mDef.toInternal (normalized);
			if (inner <= 0.0)
				std::snprintf (text, sizeof (text), "off");
			else
				std::snprintf (text, sizeof (text), "%+.1f dB", outputDecibels (inner));
			UString (string, str16BufferSize (String128)).assign (USTRING (text));
			return;
		}

		// The ten phase controls read in degrees, as the panel does.
		if (mIsPhase)
		{
			formatPhase (phaseDegrees (mDef.toInternal (normalized)), text, sizeof (text));
			UString (string, str16BufferSize (String128)).assign (USTRING (text));
			appendDegreeSign (string);
			return;
		}

		switch (mDef.type)
		{
			case ParamType::Bool:
				std::snprintf (text, sizeof (text), "%s", (normalized >= 0.5) ? "On" : "Off");
				break;

			case ParamType::Enum:
			{
				// Every enum in this plug-in is a filter-shape selector. Low
				// pass is Off/On; the other three are Off/Add/Mult/Max.
				const int step = static_cast<int> (mDef.toInternal (normalized));
				if (mDef.id == kLowPassState)
					std::snprintf (text, sizeof (text), "%s",
					               kLowPassStateNames[std::max (0, std::min (1, step))]);
				else
					std::snprintf (text, sizeof (text), "%s",
					               kFilterStateNames[std::max (0, std::min (3, step))]);
				break;
			}

			case ParamType::Int:
				std::snprintf (text, sizeof (text), "%d",
				               static_cast<int> (mDef.toInternal (normalized)));
				break;

			case ParamType::Float:
			default:
				std::snprintf (text, sizeof (text), "%.1f", mDef.toPlain (normalized));
				break;
		}

		UString (string, str16BufferSize (String128)).assign (USTRING (text));
	}

	/** Type a time in and have it land where the panel would put it. Without
	    this the base class would read "200 ms" as 200 % and clamp it to the
	    top of the control. */
	bool fromString (const TChar* string, ParamValue& normalized) const SMTG_OVERRIDE
	{
		if (mDef.id == kOutputLevel)
		{
			char text[64] = {};
			UString (const_cast<TChar*> (string), tstrlen (string)).toAscii (text, sizeof (text));
			char* end = nullptr;
			const double db = std::strtod (text, &end);
			if (end == text)
				return false;
			normalized = outputControlFromDb (db);
			return true;
		}

		if (mIsPhase)
		{
			char text[64] = {};
			UString (const_cast<TChar*> (string), tstrlen (string)).toAscii (text, sizeof (text));
			// toAscii drops the degree sign, which is all we want gone.
			char* end = nullptr;
			const double degrees = std::strtod (text, &end);
			if (end == text)
				return false;
			normalized = phaseToInternal (degrees);
			return true;
		}

		if (mTimeRole == TimeRole::None)
			return RangeParameter::fromString (string, normalized);

		// The internal range is what envelopeTimeToInternal returns, and for
		// every one of these parameters it is 0..1 - the same numbers as the
		// normalized range. Anything else and this mapping would be wrong,
		// so hand it back to the base class rather than guess.
		if (mDef.internalMin != 0.0 || mDef.internalMax != 1.0)
			return RangeParameter::fromString (string, normalized);

		char text[64] = {};
		UString (const_cast<TChar*> (string), tstrlen (string)).toAscii (text, sizeof (text));

		double seconds = 0.0;
		if (!parseEnvelopeTime (text, seconds))
			return false;

		normalized = envelopeTimeToInternal (mDef.id, seconds);
		return true;
	}

private:
	/** Put a degree sign on the end of a String128. It is U+00B0, which is
	    not ASCII - and USTRING()/UString::assign only carry ASCII across -
	    so it has to be written as a UTF-16 code unit by hand. */
	static void appendDegreeSign (String128 string)
	{
		int n = 0;
		while (n < 126 && string[n] != 0)
			++n;
		string[n] = 0x00B0;
		string[n + 1] = 0;
	}

	ParamDef mDef;
	TimeRole mTimeRole;
	bool     mIsPhase;
};

} // anonymous namespace

//------------------------------------------------------------------------
tresult PLUGIN_API ForTranController::initialize (FUnknown* context)
{
	tresult result = EditControllerEx1::initialize (context);
	if (result != kResultOk)
		return result;

	for (int i = 0; i < kNumParams; ++i)
		parameters.addParameter (new ForTranParameter (kParams[i]));

	// Standard VST3 bypass, separate from the DXi's own "Enabled" switch -
	// which, unlike SpaceDub's, defaults ON.
	parameters.addParameter (STR16 ("Bypass"), nullptr, 1, 0,
	                         ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass, kBypass);

	// MIDI controller destinations. The host writes these through
	// IMidiMapping; they are live controller state, not settings, so they
	// are deliberately absent from getState/setState.
	parameters.addParameter (STR16 ("MIDI Volume"), nullptr, 0, 1.0,
	                         ParameterInfo::kNoFlags, kMidiVolume);
	parameters.addParameter (STR16 ("MIDI Pan"), nullptr, 0, 0.5,
	                         ParameterInfo::kNoFlags, kMidiPan);
	parameters.addParameter (STR16 ("Pitch Bend"), nullptr, 0, 0.5,
	                         ParameterInfo::kNoFlags, kMidiPitchBend);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API ForTranController::setComponentState (IBStream* state)
{
	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);

	int32 version = 0;
	if (!streamer.readInt32 (version))
		return kResultFalse;

	int32 count = 0;
	if (!streamer.readInt32 (count))
		return kResultFalse;

	for (int32 i = 0; i < count; ++i)
	{
		double v = 0.0;
		if (!streamer.readDouble (v))
			return kResultFalse;
		if (i < kNumParams)
			setParamNormalized (static_cast<ParamID> (i), v);
	}

	// Anything the stream is too short to mention goes back to its default -
	// see the same loop in ForTranProcessor::setState.
	for (int32 i = count; i < kNumParams; ++i)
		setParamNormalized (static_cast<ParamID> (i), kParams[i].defaultNormalized ());

	int32 bypass = 0;
	if (streamer.readInt32 (bypass))
		setParamNormalized (kBypass, bypass ? 1.0 : 0.0);

	return kResultOk;
}

//------------------------------------------------------------------------
void ForTranController::requestWaveFrame ()
{
	if (auto* message = allocateMessage ())
	{
		FReleaser releaser (message);
		message->setMessageID (kForTranWaveRequestMessage);
		sendMessage (message);
	}
}

//------------------------------------------------------------------------
tresult PLUGIN_API ForTranController::notify (IMessage* message)
{
	if (message && FIDStringsEqual (message->getMessageID (), kForTranWaveDataMessage))
	{
		const void* data = nullptr;
		uint32 bytes = 0;
		if (message->getAttributes ()->getBinary (kForTranWaveAttribute, data, bytes) == kResultOk &&
		    data && bytes >= sizeof (float))
		{
			const int n = static_cast<int> (bytes / sizeof (float));
			for (auto* editor : mEditors)
				editor->setLiveWaveform (static_cast<const float*> (data), n);
		}
		return kResultOk;
	}

	if (message && FIDStringsEqual (message->getMessageID (), kForTranSampleRateMessage))
	{
		double sampleRate = 0.0;
		if (message->getAttributes ()->getFloat (kForTranSampleRateAttribute, sampleRate) == kResultOk &&
		    sampleRate > 1000.0)
		{
			mHostSampleRate = sampleRate;
		}
		return kResultOk;
	}
	return EditControllerEx1::notify (message);
}

//------------------------------------------------------------------------
tresult PLUGIN_API ForTranController::getMidiControllerAssignment (
    int32 /*busIndex*/, int16 /*channel*/, CtrlNumber midiControllerNumber, ParamID& id)
{
	switch (midiControllerNumber)
	{
		case kCtrlVolume: id = kMidiVolume;    return kResultTrue;
		case kCtrlPan:    id = kMidiPan;       return kResultTrue;
		case kPitchBend:  id = kMidiPitchBend; return kResultTrue;
		default: break;
	}
	return kResultFalse;
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API ForTranController::createView (FIDString name)
{
	if (name && FIDStringsEqual (name, ViewType::kEditor))
		return new ForTranEditor (this);
	return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API ForTranController::setParamNormalized (ParamID tag, ParamValue value)
{
	const tresult result = EditControllerEx1::setParamNormalized (tag, value);
	if (result != kResultOk)
		return result;

	for (auto* editor : mEditors)
		editor->updateControl (tag, value);

	return result;
}

//------------------------------------------------------------------------
void ForTranController::editorAttached (EditorView* editor)
{
	if (auto* e = dynamic_cast<ForTranEditor*> (editor))
		if (std::find (mEditors.begin (), mEditors.end (), e) == mEditors.end ())
			mEditors.push_back (e);
}

//------------------------------------------------------------------------
void ForTranController::editorRemoved (EditorView* editor)
{
	editorDestroyed (editor);
}

//------------------------------------------------------------------------
void ForTranController::editorDestroyed (EditorView* editor)
{
	// Do NOT dynamic_cast here. EditorView::~EditorView() is one of the two
	// callers, and by then the ForTranEditor sub-object is gone, so the cast
	// yields null and the entry survives as a dangling pointer - which the
	// next setParamNormalized above would then walk. Comparing upcast
	// pointers is well defined at every point in the destruction sequence.
	// See PORTING-GUIDE section 7; this one cost real time on SpaceDub.
	mEditors.erase (std::remove_if (mEditors.begin (), mEditors.end (),
	                                [editor] (ForTranEditor* e) {
		                                return static_cast<EditorView*> (e) == editor;
	                                }),
	                mEditors.end ());
}

//------------------------------------------------------------------------
} // namespace ForTran
