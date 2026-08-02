// Integration tests for the CSP server

#include "../src/config.h"
#include "../src/log.h"
#include "../src/proxy.h"
#include "helpers.h"
#include "test.h"
#include <stdlib.h>
#include "helpers.h"
#include "test.h"
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

charon_config config_for_test() {
  charon_config config = {.aap2_address = "tcp://0.0.0.0:4226",
                          .remote_eid = "dtn://bob.dtn/charon",
                          .secret_name = "D3TN_AAP2_KEY",
                          .interface_type = CSP,
                          .csp_port = 10,
                          .can_interface = "vcan0",
                          .proxy_role = CHARON_LISTEN,
                          .peer_csp_address = 2,
                          .local_csp_address = 1};
  putenv("D3TN_AAP2_KEY=my_extremely_secret_secret_omg_i_love_this_secretly_"
         "secret_secret");
  return config;
}

int test_can_interface_exists() {
  charon_config config = config_for_test();
  int interface_status = assert_can_interface_exists(config.can_interface);
  ASSERT_TRUE(interface_status >= 0);
  TEST_PASS();
  return 1;
}

int test_can_interface_not_exists() {
  int interface_status = assert_can_interface_exists("randomstuff");
  ASSERT_TRUE(interface_status < 0);
  TEST_PASS();
  return 1;
}

int test_should_init_proxy() {
  charon_proxy proxy;
  charon_config config = config_for_test();

  int config_status = charon_proxy_init(&proxy, &config);
  ASSERT_EQ(config_status, 1);
  TEST_PASS();
  return 1;
}

// Wrapper for csp_proxy_listen that can be stopped with a flag
typedef struct {
  csp_handler_args *handler_args;
  volatile int *running;
} server_thread_args;

static void *run_server_listen(void *arg) {
  server_thread_args *args = (server_thread_args *)arg;
  
  while (*(args->running)) {
    csp_socket_t *sock = csp_socket(CSP_SO_NONE);
    csp_bind(sock, args->handler_args->config->csp_port);
    csp_listen(sock, 10);
    
    // Use a short timeout so we can check the running flag frequently
    csp_conn_t *conn = csp_accept(sock, 100); // 100ms timeout
    if (conn != NULL) {
      // Connection accepted - spawn handler threads as the real implementation does
      csp_thread_args *rx_args = malloc(sizeof(csp_thread_args));
      rx_args->handler = csp_to_bpa;
      rx_args->context = args->handler_args;
      rx_args->conn = conn;
      
      pthread_t rx_thread;
      pthread_create(&rx_thread, NULL, csp_handler_trampoline, rx_args);
      
      // Run tx handler in current thread
      bpa_to_csp(args->handler_args, conn);
      
      csp_close(conn);
      pthread_join(rx_thread, NULL);
      
      // After handling one connection, stop the server for the test
      break;
    }
    csp_close(sock);
  }
  
  free(arg);
  return NULL;
}

int test_should_answer_as_server() {
  charon_proxy proxy;
  charon_config config = config_for_test();
  
  if (charon_proxy_init(&proxy, &config) < 0) {
    TEST_FAIL("Failed to initialize proxy");
  }

  csp_handler_args handler_args = {
      .proxy = &proxy, .config = &config, .csp_conn = NULL};

  // Flag to control server thread
  volatile int running = 1;

  // Launch server listener in a background thread
  server_thread_args *thread_args = malloc(sizeof(server_thread_args));
  thread_args->handler_args = &handler_args;
  thread_args->running = &running;
  
  pthread_t server_thread;
  if (pthread_create(&server_thread, NULL, run_server_listen, thread_args) != 0) {
    free(thread_args);
    close_aap2(proxy.dtn_tx_interface);
    close_aap2(proxy.dtn_rx_interface);
    TEST_FAIL("Failed to create server thread");
  }

  // Give the server thread time to start listening
  usleep(500000); // 500ms

  // Now act as a client and connect to the server
  csp_conn_t *client_conn = csp_connect(CSP_PRIO_NORM, config.local_csp_address, 
                                        config.csp_port, 1000, CSP_O_NONE);
  if (client_conn == NULL) {
    running = 0;
    pthread_join(server_thread, NULL);
    close_aap2(proxy.dtn_tx_interface);
    close_aap2(proxy.dtn_rx_interface);
    TEST_FAIL("Failed to connect as client - server may not be listening");
  }

  // Send a test message
  const char *test_msg = "Hello from test client";
  csp_packet_t *packet = csp_buffer_get(strlen(test_msg) + 1);
  if (packet == NULL) {
    csp_close(client_conn);
    running = 0;
    pthread_join(server_thread, NULL);
    close_aap2(proxy.dtn_tx_interface);
    close_aap2(proxy.dtn_rx_interface);
    TEST_FAIL("Failed to get CSP buffer");
  }
  
  memcpy(packet->data, test_msg, strlen(test_msg) + 1);
  packet->length = strlen(test_msg) + 1;
  
  // Send message to server
  if (!csp_send(client_conn, packet, 1000)) {
    csp_buffer_free(packet);
    csp_close(client_conn);
    running = 0;
    pthread_join(server_thread, NULL);
    close_aap2(proxy.dtn_tx_interface);
    close_aap2(proxy.dtn_rx_interface);
    TEST_FAIL("Failed to send test message");
  }
  
  csp_buffer_free(packet);
  csp_close(client_conn);

  // Give server thread time to process the message
  usleep(200000);
  
  // Signal server to stop
  running = 0;
  pthread_join(server_thread, NULL);
  
  // Clean up proxy resources
  close_aap2(proxy.dtn_tx_interface);
  close_aap2(proxy.dtn_rx_interface);
  
  TEST_PASS();
  return 1;
}

int main() {
  log_warn("This test suite needs the ud3tn containers to be running.\nTo do "
           "so, just "
           "run : \ndocker compose up");
  if (test_can_interface_exists() < 0) {
    TEST_FAIL("Interface vcan0 doesn't exist");
  }

  if (test_can_interface_not_exists() < 0) {
    TEST_FAIL("Interface randomstuff exist but shouldn't");
  }

  if (test_should_init_proxy() < 0) {
    TEST_FAIL("Couldn't initialize proxy");
  }

  if (test_should_answer_as_server() < 0) {
    TEST_FAIL("Server test failed");
  }

  exit(0);
  return 0;
}
