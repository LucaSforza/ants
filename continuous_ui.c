#include "continuous_app.h"

#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <raylib.h>

#define VIEW_SIZE 900
#define PANEL_SIZE 150
#define TRAIL_CAPACITY 8192

static Color field_color(double c) {
  /* Dark blue -> cyan -> warm yellow, with monotonically increasing lightness. */
  if (c < .5) return (Color){(unsigned char)(12+40*c),
                             (unsigned char)(18+280*c),
                             (unsigned char)(38+280*c), 255};
  return (Color){(unsigned char)(32+446*(c-.5)),
                 (unsigned char)(158+174*(c-.5)),
                 (unsigned char)(178-276*(c-.5)), 255};
}

static void update_field(const Continuous *s, Color *pixels, Texture2D texture) {
  int n = s->p.resolution;
  /* Duplicate periodic edge: GPU bilinear filtering also agrees at the seam. */
  for (int j = 0; j <= n; j++)
    for (int i = 0; i <= n; i++)
      pixels[i+(n+1)*j] = field_color(continuous_color(s->phase[i%n+n*(j%n)]));
  UpdateTexture(texture, pixels);
}

int continuous_window(const ContinuousOptions *o) {
  Continuous s;
  if (!continuous_init(&s, o->p)) {
    fputs("Cannot allocate simulation field\n", stderr);
    return EXIT_FAILURE;
  }
  int n = s.p.resolution;
  Color *pixels = malloc((size_t)(n+1)*(n+1)*sizeof(Color));
  if (!pixels) { continuous_free(&s); return EXIT_FAILURE; }
  InitWindow(VIEW_SIZE, VIEW_SIZE + PANEL_SIZE, "Continuous Langton Ant");
  if (!IsWindowReady()) { free(pixels); continuous_free(&s); return EXIT_FAILURE; }
  SetTargetFPS(60);
  Image image = {.data = pixels, .width = n+1, .height = n+1, .mipmaps = 1,
                 .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
  /* Initialize image before first upload; no undefined pixel reads. */
  for (size_t i = 0; i < (size_t)(n+1)*(n+1); i++) pixels[i] = field_color(0);
  Texture2D texture = LoadTextureFromImage(image);
  if (!IsTextureValid(texture)) {
    CloseWindow(); free(pixels); continuous_free(&s); return EXIT_FAILURE;
  }
  SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
  Vector2 trail[TRAIL_CAPACITY];
  size_t count = 0, head = 0;
  uint64_t stride = (uint64_t)fmax(1, ceil(.04/s.p.dt));
  double rate = o->rate, accumulator = 0;
  bool paused = false, show_trail = o->trail, dirty = true;
  float scale = VIEW_SIZE / s.p.size;
  while (!WindowShouldClose()) {
    if (IsKeyPressed(KEY_SPACE)) paused = !paused;
    if (IsKeyPressed(KEY_T)) show_trail = !show_trail;
    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD)) rate = fmin(rate*2, 1024);
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT)) rate = fmax(rate/2, 1.0/1024);
    if (IsKeyPressed(KEY_R)) {
      continuous_reset(&s);
      accumulator = 0; count = head = 0; dirty = true;
    }
    if (!paused && (!o->steps || s.steps < o->steps))
      accumulator += GetFrameTime() * rate;
    bool single = paused && IsKeyPressed(KEY_N);
    double start = GetTime();
    /* Fixed dt regardless of FPS/rate; preserve backlog under overload.
       A wall-time work budget keeps controls responsive, not a changed dt. */
    while ((single || (!paused && accumulator >= s.p.dt)) &&
           (!o->steps || s.steps < o->steps)) {
      continuous_step(&s);
      if (!single) accumulator -= s.p.dt;
      single = false;
      dirty = true;
      if (s.steps % stride == 0) {
        trail[head] = (Vector2){s.x, s.y};
        head = (head+1) % TRAIL_CAPACITY;
        if (count < TRAIL_CAPACITY) count++;
      }
      if (GetTime() - start > .010) break;
    }
    if (dirty) { update_field(&s, pixels, texture); dirty = false; }
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(texture, (Rectangle){.5f, .5f, n, n},
                   (Rectangle){0, 0, VIEW_SIZE, VIEW_SIZE}, (Vector2){0, 0}, 0, WHITE);
    if (show_trail) {
      for (size_t k = 1; k < count; k++) {
        Vector2 a = trail[(head+TRAIL_CAPACITY-count+k-1)%TRAIL_CAPACITY];
        Vector2 b = trail[(head+TRAIL_CAPACITY-count+k)%TRAIL_CAPACITY];
        /* Break, rather than draw a spurious line across the torus seam. */
        if (fabs(a.x-b.x) > s.p.size/2 || fabs(a.y-b.y) > s.p.size/2) continue;
        DrawLineV((Vector2){a.x*scale, a.y*scale},
                  (Vector2){b.x*scale, b.y*scale}, (Color){255, 135, 80, 160});
      }
    }
    /* Periodic copies of the marker are clipped to the simulation viewport. */
    BeginScissorMode(0, 0, VIEW_SIZE, VIEW_SIZE);
    for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
      Vector2 a = {s.x*scale+i*VIEW_SIZE, s.y*scale+j*VIEW_SIZE};
      Vector2 b = {a.x+13*cos(s.theta), a.y+13*sin(s.theta)};
      DrawCircleV(a, 4.5f, RED);
      DrawLineEx(a, b, 2, WHITE);
    }
    EndScissorMode();
    DrawRectangle(0, VIEW_SIZE, VIEW_SIZE, PANEL_SIZE, (Color){18, 20, 26, 255});
    DrawText(TextFormat("t = %.3f   steps = %" PRIu64 "   rate = %.3gx   %s",
                        s.time, s.steps, rate, paused ? "PAUSED" :
                        (o->steps && s.steps >= o->steps ? "DONE" : "RUNNING")),
             12, VIEW_SIZE+10, 20, GREEN);
    DrawText(TextFormat("v=%.4g  omega=%.4g  kappa=%.4g  sigma=%.4g",
                        s.p.speed, s.p.omega, s.p.kappa, s.p.sigma),
             12, VIEW_SIZE+38, 18, WHITE);
    DrawText(TextFormat("N=%d  L=%.4g  dx=%.4g  dt=%.4g  backlog=%.3gs",
                        n, s.p.size, s.p.size/n, s.p.dt, accumulator),
             12, VIEW_SIZE+62, 18, accumulator > 1 ? ORANGE : LIGHTGRAY);
    DrawText("Space pause/resume | R reset | +/- rate | T trail | N step (paused)",
             12, VIEW_SIZE+88, 18, LIGHTGRAY);
    DrawText("Field: dark blue c=0 -> cyan c=0.5 -> yellow c=1 | red: ant",
             12, VIEW_SIZE+114, 18, LIGHTGRAY);
    EndDrawing();
  }
  UnloadTexture(texture);
  CloseWindow();
  free(pixels);
  continuous_free(&s);
  return EXIT_SUCCESS;
}
