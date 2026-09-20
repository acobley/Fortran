//------------------------------------------------------------------------
// ForTran - the component state, written and read in one place
//------------------------------------------------------------------------

#include "ForTranState.h"

#include "base/source/fstreamer.h"

using namespace Steinberg;

namespace ForTran {

//------------------------------------------------------------------------
bool writeComponentState (IBStream* state, const double* params, bool bypass)
{
	if (!state || !params)
		return false;

	IBStreamer streamer (state, kLittleEndian);

	if (!streamer.writeInt32 (kStateVersion))
		return false;
	if (!streamer.writeInt32 (kNumParams))
		return false;
	for (int i = 0; i < kNumParams; ++i)
		if (!streamer.writeDouble (params[i]))
			return false;
	return streamer.writeInt32 (bypass ? 1 : 0);
}

//------------------------------------------------------------------------
bool readComponentState (IBStream* state, double* params, bool& bypass)
{
	if (!state || !params)
		return false;

	IBStreamer streamer (state, kLittleEndian);

	int32 version = 0;
	if (!streamer.readInt32 (version))
		return false;

	int32 count = 0;
	if (!streamer.readInt32 (count))
		return false;

	for (int32 i = 0; i < count; ++i)
	{
		double v = 0.0;
		if (!streamer.readDouble (v))
			return false;
		if (i < kNumParams)
			params[i] = v;
	}

	// A project - or a preset - saved before a parameter existed carries a
	// shorter stream. Everything it does not mention goes back to its
	// default rather than keeping whatever the previous patch left in this
	// instance.
	for (int32 i = count; i < kNumParams; ++i)
		params[i] = kParams[i].defaultNormalized ();

	int32 flag = 0;
	if (streamer.readInt32 (flag))
		bypass = (flag != 0);

	return true;
}

//------------------------------------------------------------------------
} // namespace ForTran
