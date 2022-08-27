# Native example

Test the library on our local machine.

Should simply work on OS X and Linux via `run.sh`

## Convert binary file

If you have a binary output from a meter available, you can convert it by using `xxd`:

```
xxd --include ehz_bin.bin >ehz_bin.h
```
