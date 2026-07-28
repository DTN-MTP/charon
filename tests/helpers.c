#include <linux/can.h>
#include <linux/sockios.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <net/if.h>
#include "helpers.h"

int assert_can_interface_exists(char *can_interface) {
  int fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  struct ifreq ifr = {0};
  if (fd < 0) {
    close(fd);
	return -1;
  }
  if (fd < 0) {
	close(fd);
    return -1;
  }

  strcpy(ifr.ifr_name, can_interface);
  if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
    close(fd);
    return -1;
  }
  close(fd);
  return 1;
}
