#ifndef CHECKED_H
#define CHECKED_H

/* Fuzzing aid, used by run.sh in place of the fixed-size arrays inside
   SmlParser. Each array gets its own exact-size heap block, so ASan reports
   any access past its end (also through memcpy/memcmp), which it cannot do
   for an array embedded in a larger object. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

template <typename T, int N> struct Checked {
  T *d;
  Checked() : d(new T[N]) { memset(d, 0xa5, N); }
  ~Checked() { delete[] d; }
  Checked(const Checked &o) : d(new T[N]) { memcpy(d, o.d, N); }
  Checked &operator=(const Checked &o)
  {
    memcpy(d, o.d, N);
    return *this;
  }
  T &operator[](long i)
  {
    /* N itself is a valid one-past-the-end address, reading it trips ASan */
    if (i < 0 || i > N) {
      fprintf(stderr, "Checked: index %ld out of bounds for size %d\n", i, N);
      abort();
    }
    return d[i];
  }
  operator T *() { return d; }
};

#endif
