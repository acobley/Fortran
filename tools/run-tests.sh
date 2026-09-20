#!/bin/sh
#-----------------------------------------------------------------------------
# Build and run every test suite.
#
# The DSP is written free of SDK types, so five of the six suites need nothing
# but a compiler. ParamTableTests needs the VST3 headers for ParamID alone; it
# is skipped with a note rather than a failure if the SDK is not there yet.
#
#   tools/run-tests.sh            release sequence step 1
#
# Binaries go to a temporary directory and are thrown away, so nothing lands
# in the source tree.
#-----------------------------------------------------------------------------
set -e

cd "$(dirname "$0")/.."
ROOT="$(pwd)"
CXX="${CXX:-c++}"
FLAGS="-std=c++17 -O2 -Wall -Wextra -I$ROOT/source"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

FAILED=0

run () {
	name="$1"; shift
	printf '\n===== %s\n' "$name"
	if ! $CXX $FLAGS "$@" -o "$OUT/$name"; then
		echo "FAILED TO BUILD: $name" >&2
		FAILED=$((FAILED + 1))
		return
	fi
	if "$OUT/$name"; then
		:
	else
		echo "FAILED: $name" >&2
		FAILED=$((FAILED + 1))
	fi
}

S="$ROOT/source"
T="$ROOT/tests"

run adsr-tests    "$S/Adsr.cpp" "$T/AdsrTests.cpp"
run filter-tests  "$S/FilterInfo.cpp" "$T/FilterInfoTests.cpp"
run combine-tests "$S/FilterInfo.cpp" "$T/FilterCombineTests.cpp"
run voice-tests   "$S/Adsr.cpp" "$S/FilterInfo.cpp" "$S/Voice.cpp" "$T/VoiceTests.cpp"
run synth-tests   "$S/Adsr.cpp" "$S/FilterInfo.cpp" "$S/Voice.cpp" "$S/Synth.cpp" "$T/SynthTests.cpp"

# ParamTableTests is the one suite that needs the SDK, for ParamID.
SDK="$ROOT/external/vst3sdk"
if [ -d "$SDK" ]; then
	printf '\n===== param-tests\n'
	if $CXX -std=c++17 -Wall -Wextra -I"$SDK" -I"$S" \
	        "$S/ForTranParams.cpp" "$T/ParamTableTests.cpp" -o "$OUT/param-tests"; then
		"$OUT/param-tests" || { echo "FAILED: param-tests" >&2; FAILED=$((FAILED + 1)); }
	else
		echo "FAILED TO BUILD: param-tests" >&2
		FAILED=$((FAILED + 1))
	fi
else
	printf '\n===== param-tests\nskipped - no VST3 SDK at external/vst3sdk yet.\nRun ./setup-xcode.sh once to fetch it.\n'
fi

# The factory presets, checked against what is actually in installer/presets.
# Needs the SDK too - the .vstpreset is read by Steinberg's own PresetFile,
# from the source list make-presets.sh builds against.
if [ -d "$SDK" ]; then
	printf '\n===== preset-tests\n'
	SDK_SOURCES=""
	while IFS= read -r line; do
		case "$line" in ''|\#*) continue ;; esac
		SDK_SOURCES="$SDK_SOURCES $SDK/$line"
	done < "$ROOT/tools/sdk-preset-sources.txt"

	# shellcheck disable=SC2086
	if $CXX -std=c++17 -O2 -DRELEASE=1 -DNDEBUG -I"$SDK" -I"$S" \
	        "$S/ForTranParams.cpp" "$S/ForTranState.cpp" \
	        "$T/PresetTests.cpp" $SDK_SOURCES -o "$OUT/preset-tests" 2>/dev/null; then
		"$OUT/preset-tests" "$ROOT/installer/presets" \
			|| { echo "FAILED: preset-tests" >&2; FAILED=$((FAILED + 1)); }
	else
		echo "FAILED TO BUILD: preset-tests" >&2
		FAILED=$((FAILED + 1))
	fi
else
	printf '\n===== preset-tests\nskipped - no VST3 SDK at external/vst3sdk yet.\n'
fi

printf '\n'
if [ "$FAILED" -eq 0 ]; then
	echo "All suites passed."
	exit 0
fi
echo "$FAILED suite(s) FAILED."
exit 1
