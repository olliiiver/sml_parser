#!/bin/sh
# Parse all *.bin dumps of https://github.com/devZer0/libsml-testing
# Usage: run.sh [path to libsml-testing checkout]

set -e

DIR=$(cd "$(dirname "$0")" && pwd)
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

DATA=$1
if [ -z "$DATA" ]; then
  DATA="$TMP/libsml-testing"
  git clone --depth 1 -q https://github.com/devZer0/libsml-testing "$DATA"
fi

c++ -Wall -I "$DIR/../../src" "$DIR/sml_check.cpp" "$DIR/../../src/sml.cpp" \
  -o "$TMP/sml_check"

# Known deviations of single dumps (see sml_check.cpp for the options)
options() {
  case "$1" in
  DZG_DVS-7420.2V.G2_mtr1_error.bin) echo "--broken" ;;
  dzg_dwsb20_2th_3byte.bin) echo "--allow-crc-errors" ;;
  EasyMeter_Q3A_A1064V1009.bin) echo "--allow-crc-errors" ;;
  DrNeuhaus_SMARTY_ix-130.bin) echo "--no-1.8.0" ;;
  EMH_eHZ-GW8E2A500AK2.bin) echo "--no-1.8.0" ;;
  EMH_eHZ361L5R.bin) echo "--no-1.8.0" ;;
  EMH_eHZ361L5R_1.bin) echo "--no-1.8.0" ;;
  HOLLEY_DTZ541-ZDBA.bin) echo "--no-1.8.0" ;;
  esac
}

FAILED=0
for f in "$DATA"/*.bin; do
  name=$(basename "$f")
  # shellcheck disable=SC2046
  if out=$("$TMP/sml_check" $(options "$name") "$f"); then
    echo "PASS $name $(options "$name"): $out"
  else
    echo "FAIL $name: $out"
    FAILED=$((FAILED + 1))
  fi
done

echo "$FAILED file(s) failed"
[ "$FAILED" -eq 0 ]
