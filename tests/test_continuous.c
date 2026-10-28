#include "continuous.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void near(double a, double b, double tol) { assert(fabs(a-b) < tol); }

static Continuous simulation(ContinuousParams p) {
  Continuous s;
  assert(continuous_init(&s, p));
  return s;
}

static void advance(Continuous *s, double time) {
  uint64_t steps = (uint64_t)llround(time/s->p.dt);
  for (uint64_t i = 0; i < steps; i++) continuous_step(s);
}

static double state_error(const Continuous *a, const Continuous *b) {
  return hypot(continuous_delta(a->x-b->x, a->p.size),
               continuous_delta(a->y-b->y, a->p.size)) +
         fabs(continuous_delta(a->theta-b->theta, 2*ANT_PI));
}

static double field_error(const Continuous *a, const Continuous *b) {
  double sum = 0;
  int n = a->p.resolution;
  for (int j = 0; j < n; j++) for (int i = 0; i < n; i++) {
    double x = i*a->p.size/n, y = j*a->p.size/n;
    double diff = continuous_sample(a, x, y) - continuous_sample(b, x, y);
    sum += diff*diff;
  }
  return sqrt(sum/(n*n));
}

static void interpolation(void) {
  Continuous s = simulation(continuous_defaults());
  int n = s.p.resolution;
  double h = s.p.size/n;
  s.phase[1] = ANT_PI;
  s.phase[n] = ANT_PI;
  near(continuous_sample(&s, h/2, h/2), .5, 1e-14);
  near(continuous_sample(&s, h, 0), 1, 1e-14);
  near(continuous_sample(&s, s.p.size-h/2, 0), 0, 1e-14);
  s.phase[n-1] = ANT_PI;
  near(continuous_sample(&s, -h/2, 0), .5, 1e-14);
  near(continuous_sample(&s, s.p.size-h/2, 0), .5, 1e-14);
  /* Phase seam: 0 and 2pi-epsilon are both black, not white. */
  s.phase[1] = 2*ANT_PI-1e-8;
  near(continuous_sample(&s, h/2, 0), 0, 1e-14);
  continuous_free(&s);
  near(continuous_wrap(-.2, 10), 9.8, 1e-14);
  near(continuous_wrap(30, 10), 0, 1e-14);
  near(continuous_delta(9.8, 10), -.2, 1e-14);
  for (int i = 0; i < 100; i++) {
    double phi = i*.31;
    near(continuous_color(phi+ANT_PI), 1-continuous_color(phi), 2e-15);
  }
}

static void kernel(void) {
  ContinuousParams p = continuous_defaults();
  near(continuous_kernel(&p, 0, 0), 1, 1e-15);
  near(continuous_kernel(&p, 3, 0), exp(-4.5), 1e-15);
  near(continuous_kernel(&p, 4, 0), 0, 1e-15);
  near(continuous_kernel(&p, 1, 2), continuous_kernel(&p, -1, -2), 1e-15);
  near(continuous_kernel(&p, .2, .3), continuous_kernel(&p, p.size+.2, .3), 1e-14);
  for (int i = 0; i <= 1000; i++) {
    double k = continuous_kernel(&p, i*.01, 0);
    assert(k >= 0 && k <= 1);
  }
}

static void analytic(void) {
  ContinuousParams p = continuous_defaults();
  p.kappa = 0;
  Continuous a = simulation(p);
  advance(&a, 2);
  double x = p.size/2 + p.speed/p.omega*(1-cos(p.omega*2));
  double y = p.size/2 + p.speed/p.omega*sin(p.omega*2);
  near(a.x, x, 5e-5); near(a.y, y, 5e-5);
  near(continuous_delta(a.theta-(ANT_PI/2-p.omega*2), 2*ANT_PI), 0, 1e-12);
  continuous_free(&a);
  p.speed = 0; p.kappa = 2; p.dt = .02;
  a = simulation(p);
  p.dt /= 2;
  Continuous b = simulation(p);
  advance(&a, 1); advance(&b, 1);
  double exact = ANT_PI/2 - p.omega*sin(p.kappa)/p.kappa;
  double ea = fabs(continuous_delta(a.theta-exact, 2*ANT_PI));
  double eb = fabs(continuous_delta(b.theta-exact, 2*ANT_PI));
  near(a.phase[p.resolution/2 + p.resolution*(p.resolution/2)], 2, 1e-13);
  assert(ea/eb > 3.9 && ea/eb < 4.1);
  printf("stationary analytic angle errors: %.8g -> %.8g (ratio %.3f)\n", ea, eb, ea/eb);
  continuous_free(&a); continuous_free(&b);
}

static void convergence(void) {
  ContinuousParams p = continuous_defaults();
  p.dt = .01;
  Continuous a = simulation(p);
  p.dt /= 2;
  Continuous b = simulation(p);
  p.dt /= 2;
  Continuous c = simulation(p);
  advance(&a, 2); advance(&b, 2); advance(&c, 2);
  double ab = state_error(&a, &b), bc = state_error(&b, &c);
  double fab = field_error(&a, &b), fbc = field_error(&b, &c);
  printf("coupled dt differences: state %.8g -> %.8g, field RMS %.8g -> %.8g\n",
         ab, bc, fab, fbc);
  assert(bc < .6*ab && fbc < .6*fab && bc < .01);
  continuous_free(&a); continuous_free(&b); continuous_free(&c);

  p.size = 32; p.resolution = 128; p.dt = .0025;
  a = simulation(p);
  p.resolution *= 2; b = simulation(p);
  p.resolution *= 2; c = simulation(p);
  advance(&a, 2); advance(&b, 2); advance(&c, 2);
  ab = state_error(&a, &b); bc = state_error(&b, &c);
  fab = field_error(&a, &b); fbc = field_error(&b, &c);
  printf("grid differences (dx .25/.125/.0625): state %.8g -> %.8g, field RMS %.8g -> %.8g\n",
         ab, bc, fab, fbc);
  assert(bc < .6*ab && fbc < .6*fab && bc < .1);
  continuous_free(&a); continuous_free(&b); continuous_free(&c);
}

static void periodic(void) {
  ContinuousParams p = continuous_defaults();
  Continuous a = simulation(p), b = simulation(p);
  a.x = .1; a.y = p.size-.1;
  int n = p.resolution, shift = 13;
  double dx = shift*p.size/n;
  b.x = a.x+dx; b.y = continuous_wrap(a.y+dx, p.size);
  continuous_step(&a); continuous_step(&b);
  assert(a.phase[0] > 0 && a.phase[n-1] > 0 && a.phase[n*(n-1)] > 0);
  assert(a.phase[n/2+n*(n/2)] == 0);
  advance(&a, 2); advance(&b, 2);
  near(continuous_delta(b.x-a.x-dx, p.size), 0, 2e-11);
  near(continuous_delta(b.y-a.y-dx, p.size), 0, 2e-11);
  near(continuous_delta(b.theta-a.theta, 2*ANT_PI), 0, 2e-11);
  for (int j = 0; j < n; j++) for (int i = 0; i < n; i++)
    near(continuous_color(a.phase[i+n*j]),
         continuous_color(b.phase[(i+shift)%n+n*((j+shift)%n)]), 2e-11);
  continuous_reset(&a);
  near(a.time, 0, 1e-15); assert(a.steps == 0);
  for (int i = 0; i < n*n; i++) assert(a.phase[i] == 0);
  continuous_free(&a); continuous_free(&b);

  /* Maximal allowed support (4*sigma=L/2): no duplicate seam deposits. */
  p.size = 16; p.sigma = 2; p.resolution = 32; p.speed = p.omega = 0;
  a = simulation(p);
  a.x = .1; a.y = 0;
  continuous_step(&a);
  n = p.resolution;
  for (int j = 0; j < n; j++) for (int i = 0; i < n; i++) {
    double expected = p.dt*p.kappa*continuous_kernel(&p, i*p.size/n-a.x,
                                                     j*p.size/n-a.y);
    near(a.phase[i+n*j], expected, 1e-14);
  }
  continuous_free(&a);
}

static void validation_and_long_runs(void) {
  ContinuousParams p = continuous_defaults();
  assert(continuous_validate(&p) == NULL);
  p.dt = NAN; assert(continuous_validate(&p));
  p = continuous_defaults(); p.resolution = 0; assert(continuous_validate(&p));
  p = continuous_defaults(); p.sigma = 0; assert(continuous_validate(&p));
  p = continuous_defaults(); p.dt = 1; assert(continuous_validate(&p));
  p = continuous_defaults(); p.sigma = p.size; assert(continuous_validate(&p));
  p = continuous_defaults(); p.resolution = 64; assert(continuous_validate(&p));
  const char *names[] = {"orbit", "balanced", "broad", "fast-turn"};
  for (size_t i = 0; i < sizeof(names)/sizeof(*names); i++) {
    assert(continuous_preset(&p, names[i]));
    Continuous s = simulation(p);
    advance(&s, 500);
    assert(continuous_finite(&s));
    assert(s.x >= 0 && s.x < p.size && s.y >= 0 && s.y < p.size);
    near(s.time, 500, 1e-12);
    continuous_free(&s);
  }
}

int main(void) {
  interpolation(); kernel(); analytic(); convergence(); periodic();
  validation_and_long_runs();
  puts("All continuous model tests passed.");
  return 0;
}
