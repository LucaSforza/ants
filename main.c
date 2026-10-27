
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <raylib.h>

#define ROWS 100
#define COLS 100
#define CELL_SIZE 10

/* ========================= GRID OPERATIONS ===========================*/

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

void ant_print_state(void) {
  printf("x = %d\n", ant.x);
  printf("y = %d\n", ant.y);
  printf("v_x = %d\n", ant.velocity.v_x);
  printf("v_y = %d\n", ant.velocity.v_y);
}

int cell_to_index(int x, int y) {
  x = ((x % COLS) + COLS) % COLS;
  y = ((y % ROWS) + ROWS) % ROWS;

  return x + COLS * y;
}

bool is_ant(int x, int y) { return ant.x == x && ant.y == y; }
bool getvalue(int x, int y) { return grid[cell_to_index(x, y)]; }
void setvalue(int x, int y, bool value) { grid[cell_to_index(x, y)] = value; }

void printgrid(int t) {
  printf("t = %d\n", t);
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      if (is_ant(x, y)) {
        putchar('A');
      } else if (getvalue(x, y)) {
        putchar('#');
      } else {
        putchar('.');
      }
      putchar(' ');
    }
    putchar('\n');
  }
}

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

  // Poi mi muovo
  ant.x += ant.velocity.v_x;
  ant.y += ant.velocity.v_y;
}

int main(int argc, char **argv) {
  bool terminal_mode = false;
  int steps_per_frame = 1;
  int opt;

  while ((opt = getopt(argc, argv, "ts:")) != -1) {
    switch (opt) {
    case 't':
      terminal_mode = true;
      break;

    case 's': {
      char *end;
      errno = 0;
      long value = strtol(optarg, &end, 10);

      if (errno != 0 || *end != '\0' || end == optarg || value <= 0 ||
          value > INT_MAX) {
        fprintf(stderr, "Invalid steps per frame: %s\n", optarg);
        return EXIT_FAILURE;
      }

      steps_per_frame = (int)value;
      break;
    }

    default:
      fprintf(stderr, "Usage: %s [-t] [-s steps]\n", argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (optind < argc) {
    fprintf(stderr, "Unexpected argument: %s\n", argv[optind]);
    return EXIT_FAILURE;
  }

  InitWindow(COLS * CELL_SIZE, ROWS * CELL_SIZE, "Ants");
  SetTargetFPS(60);

  size_t t = 0;

  while (!WindowShouldClose()) {
    for (int i = 0; i < steps_per_frame && t < 12000; i++) {
      update_state();
      t++;
    }

    BeginDrawing();

    if (terminal_mode) {
      printgrid(t);
    } else {
      raylib_drawgrid(t);
    }

    EndDrawing();
  }

  CloseWindow();
  return EXIT_SUCCESS;
}
