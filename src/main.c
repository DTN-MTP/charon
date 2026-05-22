#include "charon.h"
#include "stdio.h"
#include <string.h>

const char *DEFAULT_CONFIG_FILE = "charon.conf";

static void show_usage(char *prog_name) {
  printf("Usage: %s <config-path>. By default config-path is set to %s.\n",
         prog_name, DEFAULT_CONFIG_FILE);
  printf("You may pass `--help` to view usage.\n");
}

int main(int argc, char *argv[]) {
  charon_config *config;
  char *prog_name = argv[0];
  if (argc == 1) {
    config = read_config(DEFAULT_CONFIG_FILE);
  } else if (argc == 2 &&
             (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h") ||
              !strcmp(argv[1], "help"))) {
    show_usage(prog_name);
    return 1;
  } else if (argc == 2) {
    config = read_config(argv[1]);
  } else {
    show_usage(prog_name);
    return 1;
  }
  if (config == NULL) {
    show_usage(prog_name);
    return 1;
  }

  charon_tunnel tunnel;
  charon_init(&tunnel, config);

  charon_run_tunnel(&tunnel, config);
}
