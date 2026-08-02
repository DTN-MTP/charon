#include <csp/arch/csp_thread.h>
#include <csp/csp.h>
#include <csp/drivers/can_socketcan.h>
#include <csp/drivers/usart.h>

// Handler function type: takes context and connection
typedef void *(*csp_handler_func)(void *context, csp_conn_t *conn);

// Internal struct to pass handler, context, and connection to threads
typedef struct {
  csp_handler_func handler;
  void *context;
  csp_conn_t *conn;
} csp_thread_args;

// Trampoline function for pthread_create - unpacks the args struct
void *csp_handler_trampoline(void *arg);

int csp_setup_route(int address);
int csp_setup_interface(char* can_device);
int csp_proxy_listen(int port, void *context, csp_handler_func rx_handler, csp_handler_func tx_handler);
int csp_proxy_send(int address, int port, void *context, csp_handler_func rx_handler, csp_handler_func tx_handler);

