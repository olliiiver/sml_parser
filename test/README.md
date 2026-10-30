# Unity tests

Test the library on our local machine (native) or on MCUs.

Execute only local tests:

```
pio test -e native
```

## libsml-testing

Parse all dumps of [libsml-testing](https://github.com/devZer0/libsml-testing)
and check that the checksums match and `1-0:1.8.0*255` provides a value:

```
./libsml-testing/run.sh
```

## Fuzzing

Feed generated and mutated SML data into the parser, built with AddressSanitizer
and UndefinedBehaviorSanitizer, see [fuzz/README.md](fuzz/README.md):

```
./fuzz/run.sh
```
