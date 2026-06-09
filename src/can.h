#include "config.h"

#define CAN_DEFAULT_TUNNEL_NAME "can0"

void can_setup_route(char *interface, int bitrate);
int can_socket_open(char *ifname);
