#include "tun.h"
#include <string.h>

const char *DEFAULT_CONFIG_FILE = "charon.conf";

int main() {
  charon_config *config = read_config(DEFAULT_CONFIG_FILE);
  if (config == NULL) {
    return 1;
 }

  int tunnel_fd = open_tunnel(config);
}
