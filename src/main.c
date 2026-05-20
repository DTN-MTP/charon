#include "charon.h"
#include <string.h>

const char *DEFAULT_CONFIG_FILE = "charon.conf";

int main(int argc, char *argv[]) {
  charon_config *config;
  if (argc == 1) {
    config = read_config(DEFAULT_CONFIG_FILE);
  } else if (argc == 2) {
    config = read_config(argv[1]);
  } else {
    return 1;
  }
  if (config == NULL) {
    return 1;
  }

  charon_tunnel tunnel;
  charon_init(&tunnel, config);

  charon_run_tunnel(&tunnel, config);
}
