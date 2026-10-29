#!/bin/sh
# Compile the parser to WebAssembly for the web demo (needs Emscripten).
# Usage: web/build.sh, then serve web/ e.g. with `python3 -m http.server -d web`

set -e

DIR=$(cd "$(dirname "$0")" && pwd)

emcc -O2 -Wall -DSML_DEBUG -DSML_NATIVE -I "$DIR/../src" \
  "$DIR/wasm_api.cpp" "$DIR/../src/sml.cpp" \
  -sMODULARIZE -sEXPORT_ES6 -sSINGLE_FILE -sENVIRONMENT=web \
  -sALLOW_MEMORY_GROWTH \
  -sEXPORTED_FUNCTIONS=_web_parse,_malloc,_free \
  -sEXPORTED_RUNTIME_METHODS=UTF8ToString,HEAPU8 \
  -o "$DIR/sml.js"
