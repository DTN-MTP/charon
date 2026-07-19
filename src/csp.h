#include <csp/arch/csp_thread.h>
#include <csp/csp.h>
#include <csp/drivers/can_socketcan.h>
#include <csp/drivers/usart.h>

int setup_route(int address);
int setup_interface(char* can_device);
int csp_proxy_listen(int port, void *rx_csp_to_bpa, void (*tx_bpa_to_csp)(void *));
int csp_proxy_send(int address, int port, void *rx_bpa_to_csp, void (*tx_csp_to_bpa)(void *));
