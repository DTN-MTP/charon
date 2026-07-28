#include "config.h"
#include "csp.h"
#include "aap2_client.h"

typedef struct {
  int port;
  int local_address;
  char *can_device;

  aap2_client *dtn_tx_interface;
  aap2_client *dtn_rx_interface;
} charon_proxy;

typedef struct {
  charon_proxy *proxy;
  csp_conn_t *csp_conn;
  charon_config *config;
} csp_handler_args;

typedef struct {
	charon_config *config;
	charon_proxy *proxy;
} listen_csp_args;

int charon_proxy_init(charon_proxy *proxy, charon_config *config);
int charon_run_proxy(charon_proxy *proxy, charon_config *config);
