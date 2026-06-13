#include "config.h"

#define CAN_DEFAULT_TUNNEL_NAME "vcan0"

void can_setup_route(char *interface, int bitrate);
int can_socket_open(char *ifname);
int can_open_tunnel(charon_config *config);
