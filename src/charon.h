#include "config.h"
#include "aap2_client.h"

typedef struct {
  // File descriptor for the network interface (e.g., tun/tap device)
  int *net_interface;
  // AAP2 client for communication with the DTN node. Later will be replaced by a more
  // generic interface to support other BPAs. (maybe bpsocket)
  aap2_client *dtn_interface;
} charon_tunnel;

int charon_forward_packet(charon_tunnel *tunnel, const charon_config *config, uint8_t *packet, int packet_size);
int charon_forward_bundle(charon_tunnel *tunnel, uint8_t *bundle, int bundle_size);
int charon_receive_packet(charon_tunnel *tunnel, uint8_t *buffer, int buffer_size);
int charon_receive_bundle(charon_tunnel *tunnel, uint8_t *buffer, int buffer_size);

int charon_run_tunnel(int *net_interface, int *dtn_interface);
int charon_close_tunnel(charon_tunnel *tunnel);
