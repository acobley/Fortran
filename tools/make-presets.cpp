//------------------------------------------------------------------------
// ForTran - build the VST3 factory presets from the Audio Unit ones
//
//   tools/make-presets.sh              after adding or re-saving a patch
//
// The patches are authored in a host - you play them, you save them - and
// Logic writes a .aupreset. This turns each of those into the matching
// .vstpreset so the VST3 ships the same factory set, WITHOUT the patch
// being designed twice or transcribed by hand.
//
// It can do that because both formats carry the same thing: the bytes
// ForTranProcessor::getState wrote. Steinberg's AU wrapper stores them in
// the plist under "Processor State" (auwrapper.mm, SaveState), and a
// .vstpreset carries them as its Comp chunk. So the conversion is: pull the
// state out of the plist, check it really is one of ours, and ask the SDK's
// own PresetFile to wrap it. Nothing is re-derived, so the two files cannot
// disagree about what a patch sounds like.
//
// What it checks, because a preset that loads silently wrong is worse than
// one that does not load:
//
//   * the plist's type/subtype/manufacturer are ForTran's, so a preset
//     saved from some other plug-in cannot be shipped as ours by accident;
//   * the state parses, and its parameter count is reported. A patch saved
//     before a parameter existed is SHORTER, and that is fine - the reader
//     defaults whatever the stream does not mention - but you should know
//     which patches will arrive with the newest controls at their defaults.
//
// A .vstpreset carries the PROCESSOR CLASS UID and a host silently ignores
// one whose UID does not match, so these files are married to the UID in
// ForTranIDs.h for good.
//------------------------------------------------------------------------

#include "ForTranIDs.h"
#include "ForTranParams.h"
#include "ForTranState.h"

#include "public.sdk/source/common/memorystream.h"
#include "public.sdk/source/vst/vstpresetfile.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <vector>

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace ForTran;

namespace {

//------------------------------------------------------------------------
// The AU identity, which must match resource/au-info.plist.
//------------------------------------------------------------------------
constexpr uint32 fourCC (const char* s)
{
	return (uint32 (uint8 (s[0])) << 24) | (uint32 (uint8 (s[1])) << 16) |
	       (uint32 (uint8 (s[2])) << 8)  |  uint32 (uint8 (s[3]));
}

constexpr uint32 kAuType         = fourCC ("aumu");   // instrument
constexpr uint32 kAuSubtype      = fourCC ("FTrn");
constexpr uint32 kAuManufacturer = fourCC ("AECo");

//------------------------------------------------------------------------
std::string readWholeFile (const std::string& path, bool& ok)
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
/** The value of <key>NAME</key> followed by <tag>value</tag>. The plists
    Logic writes are flat and regular, so this is enough - and a parser that
    is enough is better than a dependency. */
bool plistValue (const std::string& xml, const std::string& key,
                 const std::string& tag, std::string& out)
{
	const std::string needle = "<key>" + key + "</key>";
	const size_t k = xml.find (needle);
	if (k == std::string::npos)
		return false;
	const size_t open = xml.find ("<" + tag + ">", k);
	if (open == std::string::npos)
		return false;
	const size_t start = open + tag.size () + 2;
	const size_t close = xml.find ("</" + tag + ">", start);
	if (close == std::string::npos)
		return false;
	out = xml.substr (start, close - start);
	return true;
}

//------------------------------------------------------------------------
std::vector<unsigned char> base64Decode (const std::string& text)
{
	auto valueOf = [] (char c) -> int {
		if (c >= 'A' && c <= 'Z') return c - 'A';
		if (c >= 'a' && c <= 'z') return c - 'a' + 26;
		if (c >= '0' && c <= '9') return c - '0' + 52;
		if (c == '+') return 62;
		if (c == '/') return 63;
		return -1;                       // whitespace, padding, anything else
	};

	std::vector<unsigned char> out;
	int bits = 0, accumulated = 0;
	for (char c : text)
	{
		const int v = valueOf (c);
		if (v < 0)
			continue;
		accumulated = (accumulated << 6) | v;
		bits += 6;
		if (bits >= 8)
		{
			bits -= 8;
			out.push_back ((unsigned char) ((accumulated >> bits) & 0xFF));
		}
	}
	return out;
}

//------------------------------------------------------------------------
bool writeFile (const std::string& path, const void* data, size_t size)
{
	FILE* f = std::fopen (path.c_str (), "wb");
	if (!f)
	{
		std::fprintf (stderr, "make-presets: cannot write %s\n", path.c_str ());
		return false;
	}
	const bool ok = size == 0 || std::fwrite (data, 1, size, f) == size;
	std::fclose (f);
	return ok;
}

//------------------------------------------------------------------------
bool convert (const std::string& dir, const std::string& file, int& shortStates)
{
	const std::string stem = file.substr (0, file.size () - std::strlen (".aupreset"));
	const std::string path = dir + "/" + file;

	bool ok = false;
	const std::string xml = readWholeFile (path, ok);
	if (!ok)
	{
		std::fprintf (stderr, "  %-22s cannot be read\n", stem.c_str ());
		return false;
	}

	// Is it ours? The four-character codes say so, and getting this wrong
	// would ship somebody else's patch under our name.
	auto codeMatches = [&xml] (const char* key, uint32 want) {
		std::string text;
		if (!plistValue (xml, key, "integer", text))
			return false;
		return (uint32) std::strtoul (text.c_str (), nullptr, 10) == want;
	};

	if (!codeMatches ("type", kAuType) || !codeMatches ("subtype", kAuSubtype) ||
	    !codeMatches ("manufacturer", kAuManufacturer))
	{
		std::fprintf (stderr, "  %-22s is not a ForTran preset (aumu/FTrn/AECo)\n", stem.c_str ());
		return false;
	}

	std::string encoded;
	if (!plistValue (xml, "Processor State", "data", encoded))
	{
		std::fprintf (stderr, "  %-22s has no Processor State\n", stem.c_str ());
		return false;
	}

	const std::vector<unsigned char> state = base64Decode (encoded);
	if (state.size () < 12)
	{
		std::fprintf (stderr, "  %-22s Processor State is too short to be one of ours\n",
		              stem.c_str ());
		return false;
	}

	// Parse it with the reader the plug-in itself uses, so "it loads" here
	// means the same thing it will mean in a host.
	MemoryStream check;
	check.write ((void*) state.data (), (int32) state.size (), nullptr);
	check.seek (0, IBStream::kIBSeekSet, nullptr);

	std::vector<double> params ((size_t) kNumParams);
	bool bypass = false;
	if (!readComponentState (&check, params.data (), bypass))
	{
		std::fprintf (stderr, "  %-22s Processor State does not parse\n", stem.c_str ());
		return false;
	}

	// The count that was actually in the stream, for the report below.
	const int32 count = *(const int32*) (state.data () + 4);
	if (count < (int32) kNumParams)
		++shortStates;

	MemoryStream component;
	component.write ((void*) state.data (), (int32) state.size (), nullptr);
	component.seek (0, IBStream::kIBSeekSet, nullptr);

	MemoryStream preset;
	if (!PresetFile::savePreset (&preset, kForTranProcessorUID, &component))
	{
		std::fprintf (stderr, "  %-22s savePreset failed\n", stem.c_str ());
		return false;
	}

	const std::string out = dir + "/" + stem + ".vstpreset";
	if (!writeFile (out, preset.getData (), (size_t) preset.getSize ()))
		return false;

	std::printf ("  %-22s %3d parameters%s -> %s.vstpreset\n", stem.c_str (), (int) count,
	             (count < (int32) kNumParams) ? " (older build)" : "              ", stem.c_str ());
	return true;
}

} // namespace

//------------------------------------------------------------------------
int main (int argc, char** argv)
{
	const std::string dir = (argc > 1) ? argv[1] : "installer/presets";

	DIR* d = opendir (dir.c_str ());
	if (!d)
	{
		std::fprintf (stderr, "make-presets: cannot open %s\n", dir.c_str ());
		return 1;
	}

	std::vector<std::string> files;
	while (dirent* e = readdir (d))
	{
		const std::string name = e->d_name;
		if (name.size () > 9 && name.compare (name.size () - 9, 9, ".aupreset") == 0)
			files.push_back (name);
	}
	closedir (d);
	std::sort (files.begin (), files.end ());

	if (files.empty ())
	{
		std::fprintf (stderr, "make-presets: no .aupreset files in %s.\n", dir.c_str ());
		std::fprintf (stderr, "              Author the patches in a host and save them there.\n");
		return 1;
	}

	int failed = 0, shortStates = 0;
	for (const std::string& f : files)
		if (!convert (dir, f, shortStates))
			++failed;

	std::printf ("\n%d preset%s, %d converted, %d failed\n", (int) files.size (),
	             files.size () == 1 ? "" : "s", (int) files.size () - failed, failed);

	if (shortStates > 0)
		std::printf ("%d were saved before the newest parameters existed; those controls\n"
		             "load at their defaults, which for 1.1.0.0 means centred pan and no glide.\n",
		             shortStates);

	return failed == 0 ? 0 : 1;
}
