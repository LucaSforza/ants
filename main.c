// https://en.wikipedia.org/wiki/Langton%27s_ant
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <raylib.h>

#define ROWS 150
#define COLS 150
#define CELL_SIZE 6

bool grid[ROWS * COLS] = {0};

struct {
  struct {
    int v_x;
    int v_y;
  } velocity;
  int x;
  int y;
} ant = {.x = ROWS / 2,
         .y = COLS / 2,
         .velocity = {
             .v_x = 0,
             .v_y = 1,
         }};

int cell_to_index(int x, int y) {
  x = ((x % COLS) + COLS) % COLS;
  y = ((y % ROWS) + ROWS) % ROWS;

  return x + COLS * y;
}

bool is_ant(int x, int y) { return ant.x == x && ant.y == y; }
bool getvalue(int x, int y) { return grid[cell_to_index(x, y)]; }
void setvalue(int x, int y, bool value) { grid[cell_to_index(x, y)] = value; }

void raylib_drawgrid(size_t t) {
  ClearBackground(BLACK);

  int offset_x = (GetScreenWidth() - COLS * CELL_SIZE) / 2;
  int offset_y = (GetScreenHeight() - ROWS * CELL_SIZE) / 2;

  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      if (getvalue(x, y)) {
        DrawRectangle(offset_x + x * CELL_SIZE, offset_y + y * CELL_SIZE,
                      CELL_SIZE, CELL_SIZE, WHITE);
      }
    }
  }

  // Formica rossa
  DrawCircle(
      offset_x + ((ant.x % COLS + COLS) % COLS) * CELL_SIZE + CELL_SIZE / 2,
      offset_y + ((ant.y % ROWS + ROWS) % ROWS) * CELL_SIZE + CELL_SIZE / 2,
      CELL_SIZE * 0.4f, RED);

  // Contatore degli step
  DrawText(TextFormat("Step: %zu", t), 10, 10, 24, GREEN);
}

void update_state(void) {

  bool current_value = getvalue(ant.x, ant.y);

  // Cambiamo il valore della cella dove si trova la formica

  setvalue(ant.x, ant.y, !current_value);

  // Possiamo pensare alla velocita` come un numero
  // complesso:
  //
  //  V = a + bi
  //
  // Inizialmente la formica punta a nord. quindi a = 0 e b = 1, V = i
  //
  // Poi se la formica si trova su una cella nera, allora si sposta verso dentra
  // (clockwise) Quindi dobbiamo moltiplicare alla Velocita` ad -i.
  //
  // -iV = b - ai
  //
  // Se invece si trova su una casella bianca allora la formica ruota a sinistra
  // (counterclockwise) moltiplicando la sua velocia` per i
  //
  // iV = -b + ai
  //
  //  Quindi possiamo vedere il ruotamento della formica semplicemente come uno
  //  scambio di variabili e negazione di una delle due parti (dipendentemente
  //  dove vuoi girare).
  //
  if (current_value) {
    int old_v_x = ant.velocity.v_x;
    ant.velocity.v_x = -ant.velocity.v_y;
    ant.velocity.v_y = old_v_x;
  } else {
    int old_v_x = ant.velocity.v_x;
    ant.velocity.v_x = ant.velocity.v_y;
    ant.velocity.v_y = -old_v_x;
  }

  // Dopo aver cambiato la direzione, possiamo andare avanti
  ant.x += ant.velocity.v_x;
  ant.y += ant.velocity.v_y;
}

int main(int argc, char **argv) {
  int steps_per_frame = 1;
  size_t T = 12000;
  int opt;

  while ((opt = getopt(argc, argv, "t:s:")) != -1) {
    switch (opt) {
    case 't':
    case 's': {
      char *end;
      errno = 0;

      long value = strtol(optarg, &end, 10);

      if (errno != 0 || end == optarg || *end != '\0' || value <= 0 ||
          value > INT_MAX) {
        fprintf(stderr, "Invalid value for -%c: %s\n", opt, optarg);
        return EXIT_FAILURE;
      }

      if (opt == 't') {
        T = (size_t)value;
      } else {
        steps_per_frame = (int)value;
      }
      break;
    }

    default:
      fprintf(stderr, "Usage: %s [-t total_steps] [-s steps_per_frame]\n",
              argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (optind < argc) {
    fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
    return EXIT_FAILURE;
  }

  InitWindow(COLS * CELL_SIZE, ROWS * CELL_SIZE, "Langton's Ant");
  SetTargetFPS(60);

  size_t t = 0;

  while (!WindowShouldClose()) {
    for (int i = 0; i < steps_per_frame && t < T; i++) {
      update_state();
      t++;
    }

    BeginDrawing();
    raylib_drawgrid(t);
    EndDrawing();
  }

  CloseWindow();
  return EXIT_SUCCESS;
}
