#include "tun.h"
#include "log.h"
#include <fcntl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

void setup_route(char *interface, char *ip_address, int mtu) {
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "ip addr add %s/24 dev %s", ip_address, interface);
  log_info("Setting up route: %s", cmd);
  system(cmd);
  snprintf(cmd, sizeof(cmd), "ip link set %s mtu %d up", interface, mtu);
  log_info("Setting MTU: %s", cmd);
  system(cmd);
}

int tun_alloc(char *ifname) {
  struct ifreq ifr = {0};
  int fd = open(TUN_DEVICE, O_RDWR);
  if (fd < 0) {
    log_error("Failed to open TUN device");
    return -1;
  }

  ifr.ifr_flags = IFF_TUN | IFF_NO_PI; // IFF_NO_PI: no packet information
  strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

  if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
    log_error("Failed to create TUN interface");
    close(fd);
    return -1;
  }
  return fd;
}

int open_tunnel(charon_config *config) {
  tun_alloc(DEFAULT_TUNNEL_NAME);
  setup_route(DEFAULT_TUNNEL_NAME, config->address, config->mtu);
  return 0;
}
