#/bin/sh

rm -rf .pio/libdeps/native/src
pio run
./.pio/build/native/program