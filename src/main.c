#include "log.h"
#include "tun.h"
#include "aap2_client.h"
#include <string.h>

const char *DEFAULT_CONFIG_FILE = "charon.conf";

int main() {
  charon_config *config = read_config(DEFAULT_CONFIG_FILE);
  if (config == NULL) {
    return 1;
 }

  aap2_client* client = connect_aap2(config->aap2_address);

  log_info(client->node_eid);


  close_aap2(client);
}
