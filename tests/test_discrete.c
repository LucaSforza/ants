/* Exercise the exact legacy rule without linking the graphics layer. */
#define ANT_DISCRETE_TEST
#include "../discrete.c"

#include <assert.h>

int main(void) {
  const int first_ten[][2] = {{76,75},{76,74},{75,74},{75,75},{74,75},
                             {74,76},{75,76},{75,75},{74,75},{74,74}};
  for (int i = 0; i < 10; i++) {
    update_state();
    assert(ant.x == first_ten[i][0] && ant.y == first_ten[i][1]);
  }
  memset(grid, 0, sizeof(grid));
  ant.x = COLS/2; ant.y = ROWS/2;
  ant.velocity.v_x = 0; ant.velocity.v_y = 1;

  /* Independent direction-index reference retaining the old unwrapped state. */
  bool reference[ROWS*COLS] = {false};
  int x = COLS/2, y = ROWS/2, direction = 1;
  const int dx[] = {1,0,-1,0}, dy[] = {0,1,0,-1};
  for (int i = 0; i < 200000; i++) {
    int ix = ((x%COLS)+COLS)%COLS, iy = ((y%ROWS)+ROWS)%ROWS;
    int index = ix+COLS*iy;
    direction = (direction+(reference[index] ? 1 : 3))%4;
    reference[index] = !reference[index];
    x += dx[direction]; y += dy[direction];
    update_state();
    assert(ant.x == ((x%COLS)+COLS)%COLS && ant.y == ((y%ROWS)+ROWS)%ROWS);
    assert(ant.velocity.v_x == dx[direction] && ant.velocity.v_y == dy[direction]);
    if (i%1000 == 0) assert(memcmp(grid, reference, sizeof(grid)) == 0);
  }
  assert(memcmp(grid, reference, sizeof(grid)) == 0);
  puts("Discrete rule preserved: first 10 states and 200000-step toroidal regression passed.");
  return 0;
}
