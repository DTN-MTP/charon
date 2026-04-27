#include "config.h"
#define TUN_DEVICE "/dev/net/tun"
#define DEFAULT_TUNNEL_NAME "charon0"

void setup_route(char *ifname, char *ip_address, int mtu);
int tun_alloc(char *ifname);
int open_tunnel(charon_config *config);
