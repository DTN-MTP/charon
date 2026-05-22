#include "charon.h"
#include "log.h"
#include <bits/types/struct_timeval.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#define BUF_SIZE 2048

// send packet to DTN interface ifnet -> dtn
// must close fds on error
int charon_forward_packet(charon_tunnel *tunnel, const charon_config *config,
                          uint8_t *packet, int packet_size) {
  if (send_aap2(tunnel->dtn_tx_interface, config->remote_eid, packet,
                packet_size) < 0) {
    log_error("Failed to send packet to DTN node");
    return -1;
  }
  return 0;
}

// send bundle to ifnet interface dtn -> ifnet
int charon_forward_bundle(charon_tunnel *tunnel, uint8_t *bundle,
                          int bundle_size) {}

int charon_receive_bundle(charon_tunnel *tunnel, uint8_t *buffer,
                          int buffer_size) {
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
  if (configure_aap2(tx, 0, 0, config->secret_name, config->remote_eid) < 0) {
    return -1;
  }

  aap2_client *rx = connect_aap2(config->aap2_address, config->secret_name);
  if (configure_aap2(rx, 1, 0, config->secret_name, config->remote_eid) < 0) {
    return -1;
  }

  int tun_fd = open_tunnel(config);

  tunnel->dtn_tx_interface = tx;
  tunnel->dtn_rx_interface = rx;
  tunnel->net_interface = tun_fd;

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
  uint8_t tun_buf[BUF_SIZE];
  fd_set read_fds;
  int tun_fd = tunnel->net_interface;
  struct timeval tv;
  tv.tv_usec = 5000;

  // Set non-blocking mode
  int flags = fcntl(tun_fd, F_GETFL, 0);
  fcntl(tun_fd, F_SETFL, flags | O_NONBLOCK);
  log_info("Running tunnel");

  while (1) {
    FD_ZERO(&read_fds);
    FD_SET(tun_fd, &read_fds);

    // Fix: Use tun_fd + 1 as the first argument
    if (select(tun_fd + 1, &read_fds, NULL, NULL, &tv) < 0) {
      if (errno == EINTR)
        continue; // Interrupted by signal
      log_error("select() failed: %s", strerror(errno));
      return NULL;
    }

    if (FD_ISSET(tun_fd, &read_fds)) {
      log_info("%i\n", tun_fd);
      log_info("Got packet !");
      ssize_t nread = read(tun_fd, tun_buf, sizeof(tun_buf));
      if (nread < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          continue; // No data, retry
        }
        log_error("read() failed: %s", strerror(errno));
        return NULL;
      }
      charon_forward_packet(tunnel, config, tun_buf, nread);
    }
  }
}

void message_handler(aap2_answer *answer, int fd) {
  log_info(answer->payload);
  log_info("FD : %i", fd);
  if (write(fd, answer->payload, answer->message->adu->payload_length) <
      0) { // might need to replace write() with a more specific function
           // depending on the type of network interface
    log_error("Failed to write to network interface");
  }
}

void *_charon_listen_aap2(void *arg) {
  listen_tun_args *args = (listen_tun_args *)arg;
  charon_tunnel *tunnel = args->tunnel;
  charon_config *config = args->config;
  recv_aap2(tunnel->dtn_rx_interface, message_handler, tunnel->net_interface);
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
  pthread_create(&thread2, NULL, _charon_listen_tun, (void *)&args);

  // Wait for threads
  pthread_join(thread1, NULL);
  pthread_join(thread2, NULL);


  return 0;
}
