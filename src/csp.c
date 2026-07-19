#include "csp.h"
#include "log.h"
#include <csp/interfaces/csp_if_lo.h>

// I based my implementation on work that I did here :
// https://github.com/Courtcircuits/ccsp

int setup_route(int address) {
  csp_conf_t csp_conf;
  csp_conf_get_defaults(&csp_conf);
  csp_conf.address = address;
  int error = csp_init(&csp_conf);
  if (error != CSP_ERR_NONE) {
    log_error("csp_init() failed, error: %d", error);
    return -1;
  }

  /* Start router task with 10000 bytes of stack (priority is only supported on
   * FreeRTOS) */
  csp_route_start_task(500, 0);
  return 0;
}

int setup_interface(char *can_device) {
  csp_iface_t *default_iface = NULL;
  int error;

  /* Add loopback interface for testing without physical interface */
  csp_iflist_add(&csp_if_lo);

  if (can_device) {
    error = csp_can_socketcan_open_and_add_interface(
        can_device, CSP_IF_CAN_DEFAULT_NAME, 0, false, &default_iface);
    if (error != CSP_ERR_NONE) {
      log_error("failed to add CAN interface [%s], error: %d", can_device,
                error);
      return -1;
    }
    csp_rtable_set(CSP_DEFAULT_ROUTE, 0, default_iface, CSP_NO_VIA_ADDRESS);
  } else {
    /* Use loopback interface as default route */
    csp_rtable_set(CSP_DEFAULT_ROUTE, 0, &csp_if_lo, CSP_NO_VIA_ADDRESS);
  }

  return 0;
}

// This listenner handles the client applications
// They initiate a connection with charon and charon
// forwards their payload to the BPA
//
// On responses, charon send the answer to the app through CSP
// rx_thread CSP -> BPA
// tx_thread (main thread) BPA -> CSP
int csp_proxy_listen(int port, void *rx_csp_to_bpa,
                     void (*tx_bpa_to_csp)(void *)) {
  csp_socket_t *sock = csp_socket(CSP_SO_NONE);
  csp_bind(sock, port);
  csp_listen(sock, 10);

  while (1) {
    csp_conn_t *conn;

    if ((conn = csp_accept(sock, 10000)) == NULL) {
      continue;
    }

    pthread_t rx_thread;
    if (pthread_create(&rx_thread, NULL, rx_csp_to_bpa, conn) != 0) {
      log_error("Failed to create receiver thread");
      csp_close(conn);
      return -1;
    }

    tx_bpa_to_csp(conn);

    csp_close(conn);
    pthread_join(rx_thread, NULL);
  }

  return 0;
}

// This listener handles the server applications
// The servers should already be running before charon
// Charon connects to the CSP app and forwards incoming
// packets to the app
int csp_proxy_send(int address, int port, void *rx_bpa_to_csp,
                   void (*tx_csp_to_bpa)(void *)) {
  csp_conn_t *conn =
      csp_connect(CSP_PRIO_NORM, address, port, 1000, CSP_O_NONE);
  if (conn == NULL) {
    log_error("Connection failed");
    return -1;
  }

  pthread_t rx_thread;

  if (pthread_create(&rx_thread, NULL, rx_bpa_to_csp, conn) != 0) {
    log_error("Failed to create receiver thread");
    csp_close(conn);
    return -1;
  }

  tx_csp_to_bpa(conn);

  csp_close(conn);
  pthread_join(rx_thread, NULL);

  return 0;
}
