#include "proxy.h"
#include "csp.h"

int charon_proxy_init(charon_proxy *proxy, charon_config *config) {
  aap2_client *tx = connect_aap2(config->aap2_address, config->secret_name);
  if (configure_aap2(tx, 0, 0) < 0) {
    return -1;
  }

  aap2_client *rx = connect_aap2(config->aap2_address, config->secret_name);
  if (configure_aap2(tx, 1, 0) < 0) {
    return -1;
  }

  if (setup_route(config->local_csp_address) < 0) {
    return -1;
  }

  if (setup_interface(config->can_interface) < 0) {
    return -1;
  }

  return 1;
}


