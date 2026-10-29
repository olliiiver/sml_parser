# Web demo

Try the parser in the browser: https://olliiiver.github.io/sml_parser/

The page runs `src/sml.cpp` compiled to WebAssembly with Emscripten. `wasm_api.cpp` feeds the input byte by byte into the parser and returns the received entries and states, the `SML_DEBUG` tree log is shown as well.

It is built and deployed to GitHub Pages by [`.github/workflows/pages.yml`](../.github/workflows/pages.yml) on every push to `develop`.

## Build locally

With [Emscripten](https://emscripten.org/docs/getting_started/downloads.html) installed (e.g. `brew install emscripten`):

```sh
web/build.sh
python3 -m http.server -d web
```

Or without installing Emscripten:

```sh
docker run --rm -v "$PWD":/src -w /src emscripten/emsdk:4.0.10 web/build.sh
```

Then open http://localhost:8000. The page uses ES modules, so it does not work when opened directly as a file.

`samples.js` contains some dumps of `test/test/` for the sample buttons.
