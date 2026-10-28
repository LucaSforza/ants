#ifndef CONTINUOUS_H
#define CONTINUOUS_H

#include <stdbool.h>
#include <stdint.h>

#define ANT_PI 3.14159265358979323846

typedef struct {
  int resolution;
  double size, speed, omega, kappa, sigma, dt;
} ContinuousParams;

typedef struct {
  ContinuousParams p;
  double *phase;
  double x, y, theta;
  double time;
  uint64_t steps;
} Continuous;

ContinuousParams continuous_defaults(void);
bool continuous_preset(ContinuousParams *p, const char *name);
/* NULL means valid; otherwise a static, human-readable diagnostic. */
const char *continuous_validate(const ContinuousParams *p);
bool continuous_init(Continuous *s, ContinuousParams p);
void continuous_reset(Continuous *s);
void continuous_free(Continuous *s);
void continuous_step(Continuous *s);
double continuous_wrap(double x, double period);
double continuous_delta(double x, double period);
double continuous_color(double phase);
double continuous_kernel(const ContinuousParams *p, double dx, double dy);
double continuous_sample(const Continuous *s, double x, double y);
bool continuous_finite(const Continuous *s);

#endif
