#include "charon.h"
#include "log.h"
#include <unistd.h>

// send packet to DTN interface ifnet -> dtn
// must close fds on error
int charon_forward_packet(charon_tunnel *tunnel, const charon_config *config,
                          uint8_t *packet, int packet_size) {
  if (send_aap2(tunnel->dtn_interface, config->remote_eid , packet, packet_size) < 0) {
    log_error("Failed to send packet to DTN node");
    return -1;
  }
  return 0;
}

// send bundle to ifnet interface dtn -> ifnet
int charon_forward_bundle(charon_tunnel *tunnel, uint8_t *bundle,
                          int bundle_size) {
  if (write(*(tunnel->net_interface), bundle, bundle_size) <
      0) { // might need to replace write() with a more specific function
           // depending on the type of network interface
    log_error("Failed to write to network interface");
    return -1;
  }
  return 0;
}

int charon_receive_bundle(charon_tunnel *tunnel, uint8_t *buffer, int buffer_size)

int charon_close_tunnel(charon_tunnel *tunnel) {
  if (close(*(tunnel->net_interface)) < 0) {
    log_error("Failed to close network interface");
    return -1;
  }
  if (close_aap2(tunnel->dtn_interface) < 0) {
    log_error("Failed to close AAP2 client socket");
    return -1;
  }
  return 0;
}
