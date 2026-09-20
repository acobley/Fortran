//------------------------------------------------------------------------
// ForTran - the shipped factory presets
//
//   tools/run-tests.sh                 builds and runs this with the rest
//
// A preset that does not load is obvious. A preset that loads SILENTLY
// WRONG is not, and there are three ways to get one:
//
//   * the .vstpreset carries the wrong class UID, so hosts ignore it and
//     say nothing;
//   * the .aupreset and the .vstpreset drift apart, so the same patch name
//     sounds different in Logic and in Cubase;
//   * the state does not parse, or carries values outside 0..1, which a
//     host will happily hand to the DSP.
//
// So these run over everything in installer/presets - whatever is actually
// about to be packaged - rather than over a fixture.
//------------------------------------------------------------------------

#include "ForTranIDs.h"
#include "ForTranParams.h"
#include "ForTranState.h"

#include "public.sdk/source/common/memorystream.h"
#include "public.sdk/source/vst/vstpresetfile.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace ForTran;

static int fails = 0;
static void chk (bool ok, const std::string& what)
{ if (!ok) { std::printf ("  FAIL: %s\n", what.c_str ()); ++fails; } }

//------------------------------------------------------------------------
static std::string slurp (const std::string& path, bool& ok)
{
	ok = false;
	FILE* f = std::fopen (path.c_str (), "rb");
	if (!f)
		return {};
	std::string out;
	char buf[4096];
	size_t n;
	while ((n = std::fread (buf, 1, sizeof (buf), f)) > 0)
		out.append (buf, n);
	std::fclose (f);
	ok = true;
	return out;
}

//------------------------------------------------------------------------
static std::vector<unsigned char> base64Decode (const std::string& text)
{
	auto valueOf = [] (char c) -> int {
		if (c >= 'A' && c <= 'Z') return c - 'A';
		if (c >= 'a' && c <= 'z') return c - 'a' + 26;
		if (c >= '0' && c <= '9') return c - '0' + 52;
		if (c == '+') return 62;
		if (c == '/') return 63;
		return -1;
	};
	std::vector<unsigned char> out;
	int bits = 0, acc = 0;
	for (char c : text)
	{
		const int v = valueOf (c);
		if (v < 0) continue;
		acc = (acc << 6) | v;
		bits += 6;
		if (bits >= 8) { bits -= 8; out.push_back ((unsigned char) ((acc >> bits) & 0xFF)); }
	}
	return out;
}

//------------------------------------------------------------------------
static bool plistData (const std::string& xml, const std::string& key, std::string& out)
{
	const std::string needle = "<key>" + key + "</key>";
	const size_t k = xml.find (needle);
	if (k == std::string::npos) return false;
	const size_t open = xml.find ("<data>", k);
	if (open == std::string::npos) return false;
	const size_t close = xml.find ("</data>", open);
	if (close == std::string::npos) return false;
	out = xml.substr (open + 6, close - open - 6);
	return true;
}

//------------------------------------------------------------------------
int main (int argc, char** argv)
{
	const std::string dir = (argc > 1) ? argv[1] : "installer/presets";

	DIR* d = opendir (dir.c_str ());
	if (!d)
	{
		std::printf ("  FAIL: cannot open %s\n", dir.c_str ());
		return 1;
	}
	std::vector<std::string> stems;
	while (dirent* e = readdir (d))
	{
		const std::string name = e->d_name;
		if (name.size () > 10 && name.compare (name.size () - 10, 10, ".vstpreset") == 0)
			stems.push_back (name.substr (0, name.size () - 10));
	}
	closedir (d);
	std::sort (stems.begin (), stems.end ());

	// A check with nothing to check is broken, not satisfied.
	if (stems.empty ())
	{
		std::printf ("  FAIL: no .vstpreset files in %s - run tools/make-presets.sh\n", dir.c_str ());
		return 1;
	}

	std::printf ("%d preset%s in %s\n", (int) stems.size (), stems.size () == 1 ? "" : "s",
	             dir.c_str ());

	int shortStates = 0;

	for (const std::string& stem : stems)
	{
		bool ok = false;
		const std::string vst3 = slurp (dir + "/" + stem + ".vstpreset", ok);
		chk (ok, stem + ": the .vstpreset cannot be read");
		if (!ok) continue;

		// 1. The SDK's own reader must accept it, and the class UID must be
		//    ours - a host silently ignores a preset whose UID does not match.
		MemoryStream stream;
		stream.write ((void*) vst3.data (), (int32) vst3.size (), nullptr);
		stream.seek (0, IBStream::kIBSeekSet, nullptr);

		PresetFile preset (&stream);
		chk (preset.readChunkList (), stem + ": the chunk list does not read");
		chk (preset.getClassID () == kForTranProcessorUID,
		     stem + ": the class UID is not ForTran's - hosts would ignore this file");

		const PresetFile::Entry* comp = preset.getEntry (kComponentState);
		chk (comp != nullptr, stem + ": there is no component state chunk");
		if (!comp) continue;

		// 2. Pull the chunk out and parse it with the plug-in's own reader,
		//    so "it loads" here means what it will mean in a host.
		std::vector<unsigned char> state ((size_t) comp->size);
		stream.seek (comp->offset, IBStream::kIBSeekSet, nullptr);
		int32 read = 0;
		stream.read (state.data (), (int32) state.size (), &read);
		chk (read == (int32) state.size (), stem + ": the component chunk is short");

		MemoryStream parse;
		parse.write (state.data (), (int32) state.size (), nullptr);
		parse.seek (0, IBStream::kIBSeekSet, nullptr);

		std::vector<double> params ((size_t) kNumParams);
		bool bypass = false;
		chk (readComponentState (&parse, params.data (), bypass),
		     stem + ": the component state does not parse");

		// 3. Every value has to be one a host can hand to the DSP.
		bool sane = true;
		for (int i = 0; i < (int) kNumParams; ++i)
			if (!std::isfinite (params[(size_t) i]) || params[(size_t) i] < 0.0 ||
			    params[(size_t) i] > 1.0)
				sane = false;
		chk (sane, stem + ": a parameter is outside 0..1");

		const int32 count = (state.size () >= 8) ? *(const int32*) (state.data () + 4) : 0;
		if (count < (int32) kNumParams)
			++shortStates;

		// 4. THE ONE THAT MATTERS: the Audio Unit and the VST3 must carry
		//    the same bytes, or the same patch name sounds different in
		//    Logic and in Cubase.
		const std::string au = slurp (dir + "/" + stem + ".aupreset", ok);
		chk (ok, stem + ": there is no .aupreset beside the .vstpreset");
		if (ok)
		{
			std::string encoded;
			chk (plistData (au, "Processor State", encoded),
			     stem + ": the .aupreset has no Processor State");
			const std::vector<unsigned char> auState = base64Decode (encoded);
			chk (auState.size () == state.size () &&
			     std::memcmp (auState.data (), state.data (), state.size ()) == 0,
			     stem + ": the .aupreset and the .vstpreset carry different states");

			// And it must be present-but-empty, which is what the wrapper
			// writes and what makes it refresh the panel on load.
			std::string controller;
			chk (plistData (au, "Controller State", controller),
			     stem + ": the .aupreset has no Controller State key - the panel would not follow");
		}

		std::printf ("  %-22s %3d parameters, %4d bytes of state%s\n", stem.c_str (), (int) count,
		             (int) state.size (), (count < (int32) kNumParams) ? "  (older build)" : "");
	}

	if (shortStates > 0)
		std::printf ("\n%d saved before the newest parameters existed - those load at their\n"
		             "defaults, which is the behaviour readComponentState is there to give.\n",
		             shortStates);

	std::printf (fails ? "\n%d CHECK(S) FAILED\n" : "\nall checks passed\n", fails);
	return fails != 0;
}
