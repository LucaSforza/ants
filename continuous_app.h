#ifndef CONTINUOUS_APP_H
#define CONTINUOUS_APP_H

#include "continuous.h"

typedef struct {
  ContinuousParams p;
  uint64_t steps, every;
  double rate;
  bool trail, headless;
  const char *csv;
} ContinuousOptions;

int run_continuous(int argc, char **argv);
int continuous_window(const ContinuousOptions *options);

#endif
