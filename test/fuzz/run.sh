#!/bin/sh
# Fuzz the parser with ASan and UBSan, see README.md.
# Usage: run.sh [seconds] [jobs] [path to libsml-testing checkout]
#        run.sh --replay <files...>

set -e

DIR=$(cd "$(dirname "$0")" && pwd)
SRC="$DIR/../../src"
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# Build against a copy of the library whose internal arrays are bounds
# checked (checked.h), so ASan also sees overflows inside the parser object.
mkdir "$TMP/src"
cp "$SRC"/*.cpp "$SRC"/*.h "$DIR/checked.h" "$TMP/src/"
sed -e 's/^#include <stdbool.h>$/#include <stdbool.h>\
#include "checked.h"/' \
  -e 's/^\( *\)unsigned char nodes\[MAX_TREE_SIZE\];/\1Checked<unsigned char, MAX_TREE_SIZE> nodes;/' \
  -e 's/^\( *\)unsigned char listBuffer\[MAX_LIST_SIZE\];/\1Checked<unsigned char, MAX_LIST_SIZE> listBuffer;/' \
  "$SRC/sml.h" > "$TMP/src/sml.h"
if ! grep -q "Checked<unsigned char, MAX_LIST_SIZE> listBuffer" "$TMP/src/sml.h" ||
  ! grep -q "Checked<unsigned char, MAX_TREE_SIZE> nodes" "$TMP/src/sml.h"; then
  echo "Could not instrument src/sml.h, please update the patterns in run.sh" >&2
  exit 1
fi

# C++17: from C++20 on, left-shifting a negative value is no longer undefined
c++ -std=gnu++17 -g -O1 -fno-omit-frame-pointer \
  -fsanitize=address,undefined -fno-sanitize-recover=undefined \
  -I "$TMP/src" "$DIR/fuzz.cpp" "$TMP/src/sml.cpp" -o "$TMP/fuzz"

# print a stack trace for the aborts of checked.h and fuzz.cpp as well
export ASAN_OPTIONS=${ASAN_OPTIONS:-handle_abort=1}
export UBSAN_OPTIONS=${UBSAN_OPTIONS:-print_stacktrace=1}

if [ "$1" = "--replay" ]; then
  shift
  "$TMP/fuzz" --replay "$@"
  exit
fi

SECONDS_=${1:-60}
JOBS=${2:-2}
DATA=$3
if [ -z "$DATA" ]; then
  DATA="$TMP/libsml-testing"
  git clone --depth 1 -q https://github.com/devZer0/libsml-testing "$DATA"
fi

BASE=$(date +%s)
echo "Fuzzing for ${SECONDS_}s with $JOBS job(s), seeds $((BASE + 1))..$((BASE + JOBS))"
n=1
while [ "$n" -le "$JOBS" ]; do
  seed=$((BASE + n))
  (
    if "$TMP/fuzz" "$SECONDS_" "$seed" "$TMP/input-$seed.bin" "$DATA"/*.bin \
      > "$TMP/log-$seed.txt" 2>&1; then
      echo "seed $seed: $(tail -1 "$TMP/log-$seed.txt")"
    else
      cp "$TMP/input-$seed.bin" "fuzz-crash-$seed.bin"
      cat "$TMP/log-$seed.txt"
      echo "seed $seed: FAILED, input saved as fuzz-crash-$seed.bin"
      touch "$TMP/failed"
    fi
  ) &
  n=$((n + 1))
done
wait

if [ -e "$TMP/failed" ]; then
  echo "Reproduce with: $0 --replay fuzz-crash-<seed>.bin"
  exit 1
fi
