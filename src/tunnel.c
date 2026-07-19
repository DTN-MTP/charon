#include "tunnel.h"
#include "config.h"
#include "log.h"
#include <stdint.h>
#include <bits/types/struct_timeval.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#define BUF_SIZE 2048

// Forward declaration for message_handler
void message_handler(aap2_answer *answer, void* rx);

// send packet to DTN interface ifnet -> dtn
// must close fds on error
int charon_forward_packet(charon_tunnel *tunnel, const charon_config *config,
                          uint8_t *packet, int packet_size) {
  if (send_aap2(tunnel->dtn_tx_interface, config->remote_eid, packet,
                packet_size) < 0) {
    log_error("Failed to send packet to BPA");
    return -1;
  }
  return 0;
}

int charon_close_tunnel(charon_tunnel *tunnel) {
  if (close((tunnel->net_interface)) < 0) {
    log_error("Failed to close network interface");
    return -1;
  }
  if (close_aap2(tunnel->dtn_tx_interface) < 0) {
    log_error("Failed to close AAP2 client socket");
    return -1;
  }
  if (close_aap2(tunnel->dtn_rx_interface) < 0) {
    log_error("Failed to close AAP2 client socket");
    return -1;
  }
  return 0;
}

int charon_init(charon_tunnel *tunnel, charon_config *config) {
  aap2_client *tx = connect_aap2(config->aap2_address, config->secret_name);
  if (configure_aap2(tx, 0, 0) < 0) {
    return -1;
  }

  aap2_client *rx = connect_aap2(config->aap2_address, config->secret_name);
  if (configure_aap2(rx, 1, 0) < 0) {
    return -1;
  }

  int fd;

  switch (config->interface_type) {
  case IP:
    fd = open_tunnel(config);
    if (fd < 0) {
      return -1;
    }
    break;
  case CAN:
    fd = can_open_tunnel(config);
    if (fd < 0) {
      return -1;
    }
    break;
  default:
    log_error("Unknown tunnel type");
    return -1;
  }

  tunnel->dtn_tx_interface = tx;
  tunnel->dtn_rx_interface = rx;
  tunnel->net_interface = fd;

  return 0;
}

typedef struct {
  charon_tunnel *tunnel;
  charon_config *config;
} listen_tun_args;

void *_charon_listen_tun(void *arg) {
  listen_tun_args *args = (listen_tun_args *)arg;
  charon_tunnel *tunnel = args->tunnel;
  charon_config *config = args->config;
  uint8_t net_buf[BUF_SIZE];
  fd_set read_fds;
  int net_fd = tunnel->net_interface;

  // Set non-blocking mode
  int flags = fcntl(net_fd, F_GETFL, 0);
  fcntl(net_fd, F_SETFL, flags | O_NONBLOCK);
  log_info("Running tunnel");

  while (true) {
    struct timeval tv = {.tv_sec = 0, .tv_usec = 5000};
    FD_ZERO(&read_fds);
    FD_SET(net_fd, &read_fds);

    // Fix: Use tun_fd + 1 as the first argument
    if (select(net_fd + 1, &read_fds, NULL, NULL, &tv) < 0) {
      if (errno == EINTR)
        continue; // Interrupted by signal
      log_error("select() failed: %s", strerror(errno));
      free(args);
      return NULL;
    }

    if (FD_ISSET(net_fd, &read_fds)) {
      ssize_t nread = read(net_fd, net_buf, sizeof(net_buf));
      if (nread < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          continue; // No data, retry
        }
        log_error("read() failed: %s, net_fd : %i", strerror(errno));
      }
      charon_forward_packet(tunnel, config, net_buf, nread);
    }
  }

  free(args);
  return NULL;
}

void message_handler(aap2_answer *answer, void* rx) {
  int* fd = (int*) rx;
  if (write(*fd, answer->payload, answer->message->adu->payload_length) <
      0) { // might need to replace write() with a more specific function
           // depending on the type of network interface
    log_error("Failed to write to network interface");
  }
}

void *_charon_listen_aap2(void *arg) {
  listen_tun_args *args = (listen_tun_args *)arg;
  charon_tunnel *tunnel = args->tunnel;
  recv_aap2(tunnel->dtn_rx_interface, message_handler, (void*) &tunnel->net_interface);
  return NULL;
}

int charon_run_tunnel(charon_tunnel *tunnel, charon_config *config) {
  pthread_t thread1, thread2;

  // Prepare arguments for loop1
  listen_tun_args args = {
      .tunnel = tunnel,
      .config = config,
  };

  // Create threads
  pthread_create(&thread1, NULL, _charon_listen_aap2, (void *)&args);

  switch (config->interface_type) {
  case IP:
    pthread_create(&thread2, NULL, _charon_listen_tun, (void *)&args);
    break;
  case CAN:
    pthread_create(&thread2, NULL, _charon_listen_tun, (void *)&args);
    break;
  case CSP:
	log_error("Cannot use CSP tunnel as of now.");
	pthread_cancel(thread1);
	return -1;
  };

  // Wait for threads
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);

  return 0;
}
