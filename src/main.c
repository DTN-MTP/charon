#include "charon.h"
#include <string.h>

const char *DEFAULT_CONFIG_FILE = "charon.conf";

int main() {
  charon_config *config = read_config(DEFAULT_CONFIG_FILE);
  if (config == NULL) {
    return 1;
 }

  charon_tunnel tunnel;
  charon_init(&tunnel, config);

  charon_run_tunnel(&tunnel, config);
}
