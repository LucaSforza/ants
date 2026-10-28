#include <stdio.h>
#include <string.h>

#include "continuous_app.h"

int run_discrete(int argc, char **argv);

int main(int argc, char **argv) {
  if (argc > 1 && (strcmp(argv[1], "--continuous") == 0 ||
                   strcmp(argv[1], "-c") == 0))
    return run_continuous(argc - 1, argv + 1);
  if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
    printf("Discrete: %s [-t total_steps] [-s steps_per_frame]\n"
           "Continuous: %s --continuous [options]\n"
           "Use %s --continuous --help for model parameters.\n",
           argv[0], argv[0], argv[0]);
    return 0;
  }
  return run_discrete(argc, argv);
}
