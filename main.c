
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
  if (x < 0) {
    x = (-x) % COLS;
    x = COLS - x;
  }
  if (y < 0) {
    y = (-y) % ROWS;
    y = ROWS - y;
  }
  if (x >= COLS)
    x = x % COLS;

  if (y >= ROWS)
    y = y % ROWS;
  return x + COLS * y;
}

bool is_ant(int x, int y) { return ant.x == x && ant.y == y; }
bool getvalue(int x, int y) { return grid[cell_to_index(x, y)]; }
void setvalue(int x, int y, bool value) { grid[cell_to_index(x, y)] = value; }

void printgrid(void) {
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

void raylib_drawgrid(void) {
  ClearBackground(BLACK);
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      if (getvalue(x, y)) {
        DrawRectangle(x * CELL_SIZE, y * CELL_SIZE, CELL_SIZE, CELL_SIZE,
                      WHITE);
      }
    }
  }
}

void load_grid(const char *filename) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    fprintf(stderr, "[ERROR] cannot open '%s'\n", filename);
    exit(1);
  }
  int x, y;
  while (fscanf(f, " (%d ,%d )", &x, &y) == 2) {
    setvalue(x, y, true);
  }
  fclose(f);
}

void save_grid(const char *filename) {
  FILE *f = fopen(filename, "w");
  if (!f) {
    fprintf(stderr, "[ERROR] cannot write '%s'\n", filename);
    exit(1);
  }
  for (int i = 0; i < ROWS * COLS; i++) {
    if (grid[i])
      fprintf(f, "(%d,%d) ", i % COLS, i / COLS);
  }
  fclose(f);
}

void update_state(void) {

  // Per prima cosa cambio colore alla cella corrente
  ant_print_state();
  putchar('\n');
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
  (void)argc;
  (void)argv;

  InitWindow(COLS * CELL_SIZE, ROWS * CELL_SIZE, "Ants");
  SetTargetFPS(60);

  for (size_t t = 0; !WindowShouldClose(); t++) {
    printf("t = %lu\n", t);
    BeginDrawing();
    raylib_drawgrid();
    EndDrawing();
    update_state();
    WaitTime(.25);
  }

  CloseWindow();

  return 0;
}
