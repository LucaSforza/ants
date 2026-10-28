#include "continuous.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

ContinuousParams continuous_defaults(void) {
  return (ContinuousParams){.resolution = 512, .size = 128, .speed = 6,
                            .omega = 3, .kappa = 7.52, .sigma = 1, .dt = .01};
}

bool continuous_preset(ContinuousParams *p, const char *name) {
  *p = continuous_defaults();
  if (strcmp(name, "balanced") == 0) return true;
  if (strcmp(name, "orbit") == 0) {
    p->kappa = 0;
    return true;
  }
  if (strcmp(name, "broad") == 0) {
    p->sigma = 2;
    p->kappa = 3.76;
    p->omega = 1.5;
    return true;
  }
  if (strcmp(name, "fast-turn") == 0) {
    p->omega = 6;
    return true;
  }
  return false;
}

const char *continuous_validate(const ContinuousParams *p) {
  if (!isfinite(p->size) || !isfinite(p->speed) || !isfinite(p->omega) ||
      !isfinite(p->kappa) || !isfinite(p->sigma) || !isfinite(p->dt))
    return "parameters must be finite";
  if (p->resolution < 32 || p->resolution > 2048)
    return "resolution must be between 32 and 2048";
  if (p->size < 1 || p->size > 1e6 || p->speed < 0 || p->speed > 1e6 ||
      p->omega < 0 || p->omega > 1e6 || p->kappa < 0 || p->kappa > 1e6 ||
      p->sigma <= 0 || p->dt < 1e-9 || p->dt > 1)
    return "require 1 <= size <= 1e6, 0 <= speed/omega/kappa <= 1e6, sigma > 0, 1e-9 <= dt <= 1";
  if (p->sigma > p->size / 8)
    return "require 4*sigma <= size/2 (unique periodic kernel support)";
  if (p->size / p->resolution > p->sigma / 2)
    return "under-resolved kernel: require size/resolution <= sigma/2";
  if (p->speed * p->dt > p->sigma / 4 || p->omega * p->dt > .1 ||
      p->kappa * p->dt > .25)
    return "timestep too large: require speed*dt <= sigma/4, omega*dt <= 0.1, kappa*dt <= 0.25";
  return NULL;
}

double continuous_wrap(double x, double period) {
  double r = fmod(x, period);
  if (r < 0) r += period;
  return r >= period ? 0 : r;
}

double continuous_delta(double x, double period) {
  return continuous_wrap(x + period / 2, period) - period / 2;
}

double continuous_color(double phase) { return .5 * (1 - cos(phase)); }

/* Gaussian up to 3 sigma, C2 taper to zero at 4 sigma; peak is one. */
double continuous_kernel(const ContinuousParams *p, double dx, double dy) {
  dx = continuous_delta(dx, p->size);
  dy = continuous_delta(dy, p->size);
  double q = (dx * dx + dy * dy) / (p->sigma * p->sigma);
  if (q >= 16) return 0;
  double taper = 1;
  if (q > 9) {
    double z = (q - 9) / 7;
    taper = (1-z)*(1-z)*(1-z)*(1+3*z+6*z*z);
  }
  return exp(-q / 2) * taper;
}

bool continuous_init(Continuous *s, ContinuousParams p) {
  *s = (Continuous){.p = p};
  if (continuous_validate(&p)) return false;
  s->phase = calloc((size_t)p.resolution * p.resolution, sizeof(double));
  if (!s->phase) return false;
  continuous_reset(s);
  return true;
}

void continuous_reset(Continuous *s) {
  memset(s->phase, 0, (size_t)s->p.resolution * s->p.resolution * sizeof(double));
  s->x = s->y = s->p.size / 2;
  s->theta = ANT_PI / 2;
  s->steps = 0;
  s->time = 0;
}

void continuous_free(Continuous *s) {
  free(s->phase);
  s->phase = NULL;
}

static int index_wrap(int i, int n) { return (i % n + n) % n; }

/* Midpoint stage colors are computed on demand at the four sensing nodes.
   Never interpolate modulo-reduced phases: interpolate their colors instead. */
static double sample_stage(const Continuous *s, double x, double y, double half) {
  int n = s->p.resolution;
  double h = s->p.size / n;
  double gx = continuous_wrap(x, s->p.size) / h;
  double gy = continuous_wrap(y, s->p.size) / h;
  int ix = (int)floor(gx), iy = (int)floor(gy);
  double fx = gx - ix, fy = gy - iy, result = 0;
  for (int j = 0; j < 2; j++) {
    for (int i = 0; i < 2; i++) {
      int nx = index_wrap(ix + i, n), ny = index_wrap(iy + j, n);
      double phase = s->phase[nx + n * ny];
      if (half != 0)
        phase += half * s->p.kappa *
                 continuous_kernel(&s->p, nx * h - s->x, ny * h - s->y);
      result += (i ? fx : 1-fx) * (j ? fy : 1-fy) * continuous_color(phase);
    }
  }
  return result;
}

double continuous_sample(const Continuous *s, double x, double y) {
  return sample_stage(s, x, y, 0);
}

static void deposit(Continuous *s, double x, double y) {
  if (s->p.kappa == 0) return;
  int n = s->p.resolution;
  double h = s->p.size / n, radius = 4 * s->p.sigma;
  int x0 = (int)ceil((x - radius) / h);
  int x1 = (int)floor((x + radius) / h);
  int y0 = (int)ceil((y - radius) / h);
  int y1 = (int)floor((y + radius) / h);
  /* At radius=L/2, the two boundary nodes coincide but K=0 there.
     Visit each periodic node at most once, including roundoff at that edge. */
  if (x1 - x0 >= n) x1 = x0 + n - 1;
  if (y1 - y0 >= n) y1 = y0 + n - 1;
  for (int j = y0; j <= y1; j++) {
    for (int i = x0; i <= x1; i++) {
      double k = continuous_kernel(&s->p, i * h - x, j * h - y);
      if (k == 0) continue;
      int index = index_wrap(i, n) + n * index_wrap(j, n);
      s->phase[index] = continuous_wrap(s->phase[index] + s->p.dt*s->p.kappa*k,
                                         2 * ANT_PI);
    }
  }
}

void continuous_step(Continuous *s) {
  double dt = s->p.dt;
  double turn = s->p.omega * (2 * continuous_sample(s, s->x, s->y) - 1);
  double mx = continuous_wrap(s->x + .5*dt*s->p.speed*cos(s->theta), s->p.size);
  double my = continuous_wrap(s->y + .5*dt*s->p.speed*sin(s->theta), s->p.size);
  double mt = s->theta + .5*dt*turn;
  double mid_turn = s->p.omega * (2 * sample_stage(s, mx, my, dt/2) - 1);
  deposit(s, mx, my);
  s->x = continuous_wrap(s->x + dt*s->p.speed*cos(mt), s->p.size);
  s->y = continuous_wrap(s->y + dt*s->p.speed*sin(mt), s->p.size);
  s->theta = continuous_wrap(s->theta + dt*mid_turn, 2 * ANT_PI);
  s->steps++;
  s->time = (double)s->steps * dt;
}

bool continuous_finite(const Continuous *s) {
  if (!isfinite(s->x) || !isfinite(s->y) || !isfinite(s->theta) ||
      !isfinite(s->time)) return false;
  for (size_t i = 0; i < (size_t)s->p.resolution * s->p.resolution; i++)
    if (!isfinite(s->phase[i]) || s->phase[i] < 0 || s->phase[i] >= 2*ANT_PI)
      return false;
  return true;
}
