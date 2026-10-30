# Fuzzing

Feeds generated and mutated SML data into the parser and reports memory
errors, undefined behaviour and broken internal state. It runs on the host
(macOS or Linux) and needs a C++ compiler with AddressSanitizer and
UndefinedBehaviorSanitizer support (clang or gcc) and `git`.

```
./fuzz/run.sh [seconds] [jobs] [path to libsml-testing checkout]
```

The defaults are 60 seconds and 2 parallel jobs. Without a path, the dumps of
[libsml-testing](https://github.com/devZer0/libsml-testing) are cloned to a
temporary directory and used as seeds. Every run uses new random seeds, which
are printed at the start.

## How it works

`fuzz.cpp` builds its inputs in three ways:

- **Mutation** of single frames cut from the seed dumps: bit flips,
  interesting bytes (TL bytes, escape bytes, ...), inserted, removed,
  duplicated or spliced ranges. Afterwards the checksum is usually fixed so
  the message is parsed to the end.
- **Generation** of SML messages: getListResponse messages with OBIS entries,
  nested and extended lists, multi-byte octet strings and integers of every
  size, wrapped into a frame with a valid checksum.
- **Random bytes**.

Each input is fed byte by byte into a fresh `SmlParser` and into the global
functions. Whenever a list or a value ends, every public function is called:
`smlOBISCheck`, `smlOBISEntry`, `smlOBISByUnit` and the unit helpers,
`smlOBISManufacturer` and `smlOBISServerId` with buffers of exact size, and the
OBIS string conversions.

`run.sh` builds the fuzzer with ASan and UBSan against a copy of `src/`, in
which `checked.h` replaces the arrays `listBuffer` and `nodes` of `SmlParser`.
Each array then lives in its own heap block of exact size, so an access past
its end is reported, also when it stays inside the parser object. The library
itself is not changed. If the declarations in `src/sml.h` change, adapt the
patterns in `run.sh`.

## When it finds something

The failing input is saved as `fuzz-crash-<seed>.bin` in the current directory
(ignored by git). Reproduce it with

```
./fuzz/run.sh --replay fuzz-crash-<seed>.bin
```

Please add a unit test for the fix, for example `test/test_negative`.

## Improving the fuzzer

Ideas are welcome, e.g. more message types in the generator, more checks on
the decoded values, or a libFuzzer target for coverage guided fuzzing.
