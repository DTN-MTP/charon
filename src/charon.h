#include "aap2_client.h"
#include "tun.h"

typedef struct {
  // File descriptor for the network interface (e.g., tun/tap device)
  int net_interface;
  // AAP2 client for communication with the DTN node. Later will be replaced by
  // a more generic interface to support other BPAs. (maybe bpsocket)
  aap2_client *dtn_tx_interface;
  aap2_client *dtn_rx_interface;
} charon_tunnel;

int charon_forward_packet(charon_tunnel *tunnel, const charon_config *config,
                          uint8_t *packet, int packet_size);
int charon_receive_packet(charon_tunnel *tunnel, uint8_t *buffer,
                          int buffer_size);

int charon_run_tunnel(charon_tunnel *tunnel, charon_config *config);
int charon_close_tunnel(charon_tunnel *tunnel);

int charon_init(charon_tunnel *tunnel, charon_config *config);
