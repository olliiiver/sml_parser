# Native example: read all entries

Reads values without knowing the OBIS codes of the meter at compile time:

- prints every entry the meter sends via `smlOBISEntry` (e.g. for an "info mode" to let the user pick values)
- watches OBIS codes given as strings (`smlOBISFromString`), e.g. from a configuration at runtime

Should simply work on OS X and Linux via `run.sh`
