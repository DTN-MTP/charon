#include "aap2_client.h"
#include "config.h"


typedef struct {
  int port;
  int local_address;
  char* can_device;

  aap2_client *dtn_tx_interface;
  aap2_client *dtn_rx_interface;
} charon_proxy;
