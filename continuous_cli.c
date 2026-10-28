#include "continuous_app.h"

#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(void) {
  puts("Continuous Langton ant (fixed-step coupled midpoint)\n"
       "  --preset balanced|orbit|broad|fast-turn (numeric overrides win)\n"
       "  --speed V --omega W --kappa K --sigma S --size L\n"
       "  --resolution N --dt H\n"
       "  --rate R         simulated seconds per real second (default 1)\n"
       "  --steps N        stop after N steps (GUI default: unlimited)\n"
       "  --headless       no window (default: 100000 steps)\n"
       "  --csv PATH       sample trajectory to CSV (headless only)\n"
       "  --every N        CSV sampling interval (default 100)\n"
       "  --trail          initially show bounded trajectory history\n"
       "  --help\n"
       "Window: Space pause/resume, R reset, +/- rate, T trail, N single step.");
}

static bool number(const char *text, double *out) {
  char *end;
  errno = 0;
  *out = strtod(text, &end);
  return !errno && end != text && *end == '\0' && isfinite(*out);
}

static bool count(const char *text, uint64_t *out) {
  char *end;
  if (!text[0] || text[0] == '-' || text[0] == '+') return false;
  errno = 0;
  unsigned long long value = strtoull(text, &end, 10);
  if (errno || end == text || *end || value == 0 || value > 1000000000000ULL)
    return false;
  *out = (uint64_t)value;
  return true;
}

static void csv_row(FILE *f, const Continuous *s) {
  fprintf(f, "%.17g,%" PRIu64 ",%.17g,%.17g,%.17g,%.17g\n", s->time,
          s->steps, s->x, s->y, s->theta, continuous_sample(s, s->x, s->y));
}

static int headless(const ContinuousOptions *o) {
  Continuous s;
  if (!continuous_init(&s, o->p)) {
    fputs("Cannot allocate simulation field\n", stderr);
    return EXIT_FAILURE;
  }
  FILE *csv = NULL;
  if (o->csv) {
    csv = fopen(o->csv, "w");
    if (!csv) {
      perror(o->csv);
      continuous_free(&s);
      return EXIT_FAILURE;
    }
    fputs("time,steps,x,y,theta,color\n", csv);
    csv_row(csv, &s);
  }
  double ux = 0, uy = 0, minx = 0, maxx = 0, miny = 0, maxy = 0;
  uint64_t reversals = 0;
  bool positive = false;
  for (uint64_t i = 0; i < o->steps; i++) {
    double x = s.x, y = s.y;
    continuous_step(&s);
    ux += continuous_delta(s.x - x, s.p.size);
    uy += continuous_delta(s.y - y, s.p.size);
    minx = fmin(minx, ux); maxx = fmax(maxx, ux);
    miny = fmin(miny, uy); maxy = fmax(maxy, uy);
    bool next = continuous_sample(&s, s.x, s.y) > .5;
    if (next != positive) reversals++;
    positive = next;
    if (csv && (s.steps % o->every == 0 || s.steps == o->steps)) csv_row(csv, &s);
  }
  bool ok = continuous_finite(&s);
  double sum = 0;
  size_t active = 0, cells = (size_t)s.p.resolution * s.p.resolution;
  for (size_t i = 0; i < cells; i++) {
    double c = continuous_color(s.phase[i]);
    sum += c;
    if (c > .1) active++;
  }
  printf("{\"steps\":%" PRIu64 ",\"time\":%.17g,\"x\":%.17g,\"y\":%.17g,"
         "\"theta\":%.17g,\"displacement\":%.17g,\"span_x\":%.17g,"
         "\"span_y\":%.17g,\"turn_sign_changes\":%" PRIu64 ","
         "\"mean_color\":%.17g,\"active_fraction\":%.17g,\"finite\":%s}\n",
         s.steps, s.time, s.x, s.y, s.theta, hypot(ux, uy), maxx-minx, maxy-miny,
         reversals, sum/cells, (double)active/cells, ok ? "true" : "false");
  if (csv) {
    bool failed = ferror(csv) != 0;
    if (fclose(csv) != 0) failed = true;
    if (failed) { fputs("CSV write failed\n", stderr); ok = false; }
  }
  continuous_free(&s);
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

int run_continuous(int argc, char **argv) {
  enum { PRESET = 256, SPEED, OMEGA, KAPPA, SIGMA, SIZE, RESOLUTION, DT,
         RATE, STEPS, HEADLESS, CSV, EVERY, TRAIL };
  static const struct option opts[] = {
    {"preset", required_argument, NULL, PRESET},
    {"speed", required_argument, NULL, SPEED},
    {"omega", required_argument, NULL, OMEGA},
    {"kappa", required_argument, NULL, KAPPA},
    {"sigma", required_argument, NULL, SIGMA},
    {"size", required_argument, NULL, SIZE},
    {"resolution", required_argument, NULL, RESOLUTION},
    {"dt", required_argument, NULL, DT},
    {"rate", required_argument, NULL, RATE},
    {"steps", required_argument, NULL, STEPS},
    {"headless", no_argument, NULL, HEADLESS},
    {"csv", required_argument, NULL, CSV},
    {"every", required_argument, NULL, EVERY},
    {"trail", no_argument, NULL, TRAIL},
    {"help", no_argument, NULL, 'h'}, {NULL, 0, NULL, 0}
  };
  ContinuousOptions o = {.p = continuous_defaults(), .every = 100, .rate = 1};
  double overrides[7] = {0};
  bool set[7] = {false};
  const char *preset = "balanced";
  int opt;
  optind = 1;
  while ((opt = getopt_long(argc, argv, "h", opts, NULL)) != -1) {
    if (opt >= SPEED && opt <= DT) {
      int index = opt - SPEED;
      if (!number(optarg, &overrides[index])) goto invalid;
      set[index] = true;
      continue;
    }
    switch (opt) {
    case PRESET: preset = optarg; break;
    case RATE:
      if (!number(optarg, &o.rate) || o.rate < 1.0/1024 || o.rate > 1024) goto invalid;
      break;
    case STEPS: if (!count(optarg, &o.steps)) goto invalid; break;
    case EVERY: if (!count(optarg, &o.every)) goto invalid; break;
    case HEADLESS: o.headless = true; break;
    case CSV: o.csv = optarg; break;
    case TRAIL: o.trail = true; break;
    case 'h': usage(); return EXIT_SUCCESS;
    default: usage(); return EXIT_FAILURE;
    }
  }
  if (optind < argc) {
    fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
    return EXIT_FAILURE;
  }
  if (!continuous_preset(&o.p, preset)) {
    fprintf(stderr, "Unknown preset: %s\n", preset);
    return EXIT_FAILURE;
  }
  if (set[0]) o.p.speed = overrides[0];
  if (set[1]) o.p.omega = overrides[1];
  if (set[2]) o.p.kappa = overrides[2];
  if (set[3]) o.p.sigma = overrides[3];
  if (set[4]) o.p.size = overrides[4];
  if (set[5]) {
    if (overrides[5] < 32 || overrides[5] > 2048 || floor(overrides[5]) != overrides[5]) {
      fputs("Resolution must be an integer between 32 and 2048\n", stderr);
      return EXIT_FAILURE;
    }
    o.p.resolution = (int)overrides[5];
  }
  if (set[6]) o.p.dt = overrides[6];
  const char *error = continuous_validate(&o.p);
  if (error) { fprintf(stderr, "Invalid configuration: %s\n", error); return EXIT_FAILURE; }
#ifdef ANT_HEADLESS
  o.headless = true;
#endif
  if (o.csv && !o.headless) {
    fputs("--csv requires --headless\n", stderr);
    return EXIT_FAILURE;
  }
  if (o.headless) {
    if (!o.steps) o.steps = 100000;
    return headless(&o);
  }
#ifndef ANT_HEADLESS
  return continuous_window(&o);
#else
  return EXIT_FAILURE;
#endif

invalid:
  fprintf(stderr, "Invalid value: %s\n", optarg ? optarg : "(missing)");
  return EXIT_FAILURE;
}

#ifdef ANT_HEADLESS
int main(int argc, char **argv) { return run_continuous(argc, argv); }
#endif
