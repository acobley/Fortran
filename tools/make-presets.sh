#!/bin/sh
#-----------------------------------------------------------------------------
# Build and run tools/make-presets.cpp, writing installer/presets/.
#
#   tools/make-presets.sh              after editing the patch table
#
# Needs the VST3 SDK: the .vstpreset is written by Steinberg's own
# PresetFile, so the format cannot be wrong, and that costs a handful of SDK
# sources at link time. They are listed here rather than in the tool's header
# comment because tests/PresetTests.cpp needs exactly the same set.
#-----------------------------------------------------------------------------
set -e

cd "$(dirname "$0")/.."
ROOT="$(pwd)"
SDK="${VST3_SDK_ROOT:-$ROOT/external/vst3sdk}"
CXX="${CXX:-c++}"

if [ ! -d "$SDK" ]; then
	echo "make-presets: no VST3 SDK at $SDK - run ./setup-xcode.sh once," >&2
	echo "              or set VST3_SDK_ROOT." >&2
	exit 1
fi

# The SDK sources PresetFile drags in, listed in one file because
# tests/PresetTests.cpp builds against exactly the same set.
SDK_SOURCES=""
while IFS= read -r line; do
	case "$line" in ''|\#*) continue ;; esac
	SDK_SOURCES="$SDK_SOURCES $SDK/$line"
done < "$ROOT/tools/sdk-preset-sources.txt"

# ON macOS THIS NEEDS CoreFoundation. Two of those sources - funknown.cpp
# for CFUUIDCreate, and fstring.cpp for its CFString allocator - are wrapped
# in SMTG_OS_MACOS, so they compile to nothing on Linux and the link
# succeeds there while failing on the Mac with undefined _CFUUIDCreate.
# Exactly the wrong way round for a project whose releases are built on a
# Mac and whose assistant works on Linux.
SDK_LIBS=""
if [ "$(uname -s)" = "Darwin" ]; then
	SDK_LIBS="-framework CoreFoundation"
fi

OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

mkdir -p installer/presets

# shellcheck disable=SC2086
$CXX -std=c++17 -O2 -DRELEASE=1 -DNDEBUG -I"$SDK" -I"$ROOT/source" \
	"$ROOT/source/ForTranParams.cpp" "$ROOT/source/ForTranState.cpp" \
	"$ROOT/tools/make-presets.cpp" $SDK_SOURCES $SDK_LIBS -o "$OUT/make-presets"

"$OUT/make-presets" "$ROOT/installer/presets"
