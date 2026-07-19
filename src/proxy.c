#include "proxy.h"
#include "log.h"

// Forward declaration for message_handler
void message_handler(aap2_answer *answer, void *rx);

int charon_proxy_init(charon_proxy *proxy, charon_config *config) {
  aap2_client *tx = connect_aap2(config->aap2_address, config->secret_name);
  if (configure_aap2(tx, 0, 0) < 0) {
    return -1;
  }

  aap2_client *rx = connect_aap2(config->aap2_address, config->secret_name);
  if (configure_aap2(rx, 1, 0) < 0) {
    return -1;
  }

  proxy->dtn_tx_interface = tx;
  proxy->dtn_rx_interface = rx;

  if (csp_setup_route(config->local_csp_address) < 0) {
    return -1;
  }

  if (csp_setup_interface(config->can_interface) < 0) {
    return -1;
  }

  return 1;
}

// called everytime a new csp connection is initiated
// Handler for CSP -> BPA direction (receiving from CSP, sending to BPA)
static void *csp_to_bpa(void *context, csp_conn_t *conn) {
  csp_handler_args *args = (csp_handler_args *)context;
  charon_proxy *proxy = args->proxy;
  charon_config *config = args->config;
  csp_packet_t *packet;

  while (1) {
    packet = csp_read(conn, 100);
    if (packet != NULL) {
      if (send_aap2(proxy->dtn_tx_interface, config->remote_eid, packet->data,
                    packet->length) < 0) {
        log_warn("Failed to send CSP packet to BPA");
        continue;
      }
      csp_buffer_free(packet);
    }
  }

  return NULL;
}

void message_handler(aap2_answer *answer, void *rx) {
  csp_conn_t *conn = (csp_conn_t *)rx;
  if (answer == NULL || answer->payload == NULL) {
    log_warn("Received NULL answer or payload");
    return;
  }

  csp_packet_t *packet = csp_buffer_get(256);
  if (packet == NULL) {
    log_error("Failed to get CSP buffer");
    return;
  }

  size_t len = answer->message->adu->payload_length;
  if (len > 256) {
    log_warn("Payload too large (%zu bytes), truncating to 256", len);
    len = 256;
  }
  memcpy(packet->data, answer->payload, len);
  packet->length = len;

  if (!csp_send(conn, packet, 1000)) {
    log_error("Send failed");
    csp_buffer_free(packet);
  }
}

// Handler for BPA -> CSP direction (receiving from BPA, sending to CSP)
static void *bpa_to_csp(void *context, csp_conn_t *conn) {
  csp_handler_args *args = (csp_handler_args *)context;
  charon_proxy *proxy = args->proxy;

  recv_aap2(proxy->dtn_rx_interface, message_handler, (void *)conn);
  return NULL;
}

void *_charon_proxy_listen_csp(void *arg) {
  listen_csp_args *args = (listen_csp_args *)arg;
  charon_proxy *proxy = args->proxy;
  charon_config *config = args->config;

  // Create handler args to pass as context
  csp_handler_args handler_args = {
    .proxy = proxy,
    .config = config,
    .csp_conn = NULL  // Will be set per-connection by csp_proxy_listen
  };

  if (csp_proxy_listen(config->csp_port, &handler_args, csp_to_bpa, bpa_to_csp) < 0) {
    return NULL;
  }

  return NULL;
}

void *_charon_proxy_listen_aap2(void *arg) {
	listen_csp_args *args = (listen_csp_args *)arg;
	charon_proxy *proxy = args->proxy;
	charon_config *config = args->config;
	
	// Create handler args to pass as context
	csp_handler_args handler_args = {
		.proxy = proxy,
		.config = config,
		.csp_conn = NULL
	};
	
	csp_proxy_send(config->peer_csp_address, config->csp_port, &handler_args, bpa_to_csp, csp_to_bpa);
	return NULL;
}

int charon_run_proxy(charon_proxy *proxy, charon_config *config) {
	pthread_t thread1, thread2;

	listen_csp_args listen_args = {
		.config = config,
		.proxy = proxy
	};

	// Create listener thread for incoming CSP connections
	if (pthread_create(&thread1, NULL, _charon_proxy_listen_csp, &listen_args) != 0) {
		log_error("Failed to create CSP listener thread");
		return -1;
	}

	// Create listener thread for outgoing CSP connections
	listen_csp_args send_args = {
		.config = config,
		.proxy = proxy
	};
	if (pthread_create(&thread2, NULL, _charon_proxy_listen_aap2, &send_args) != 0) {
		log_error("Failed to create CSP sender thread");
		pthread_join(thread1, NULL);
		return -1;
	}

	pthread_join(thread1, NULL);
	pthread_join(thread2, NULL);

	return 0;
}
