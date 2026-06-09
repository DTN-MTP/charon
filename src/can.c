#include "can.h"
#include "log.h"
#include <linux/can/raw.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

void can_setup_interface(char *interface, int bitrate) {
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "ip link set %s type can bitrate %i", interface,
           bitrate);
  log_info("Setting link bitrate : %s", cmd);
  system(cmd);
  snprintf(cmd, sizeof(cmd), "ip link set up %s", interface);
  log_info("Setting link up : %s", cmd);
  system(cmd);
}

int can_socket_open(char *ifname) {
  int fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  struct sockaddr_can addr;
  struct ifreq ifr = {0};
  if (fd < 0) {
    log_error("Failed to open CAN socket");
    return -1;
  }

  strcpy(ifr.ifr_name, ifname);
  if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
    log_error("Failed to connect to CAN interface");
    close(fd);
    return -1;
  }
  if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    log_error("Idk what this does but bind failed for CAN");
    close(fd);
    return -1;
  }
  return fd;
}

int can_open_tunnel(charon_config *config) {
  int fd = can_socket_open(CAN_DEFAULT_TUNNEL_NAME);
  can_setup_interface(CAN_DEFAULT_TUNNEL_NAME, config->bitrate);
  return fd;
}
