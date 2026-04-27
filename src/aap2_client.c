#include "aap2_client.h"
#include "log.h"
#include "proto/aap2.pb-c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static int recv_exact(int fd, void *buf, size_t len) {
  size_t total = 0;
  while (total < len) {
    ssize_t n = recv(fd, (uint8_t *)buf + total, len - total, 0);
    if (n <= 0)
      return -1;
    total += n;
  }

  return 0;
}

static int recv_varint(int fd, uint64_t *out) {
  *out = 0;
  for (int shift = 0; shift < 64; shift += 7) {
    uint8_t b;
    if (recv_exact(fd, &b, 1) < 0)
      return -1;
    *out |= (uint64_t)(b & 0x7F) << shift;
    if (!(b & 0x80))
      return 0;
  }
  return -1;
}

static int send_varint(int fd, uint64_t value) {
  uint8_t buf[10];
  int i = 0;
  do {
    buf[i] = value & 0x7F;
    value >>= 7;
    if (value)
      buf[i] |= 0x80;
    i++;
  } while (value);
  ssize_t nsent = send(fd, buf, i, 0);
  return (nsent == i) ? 0 : -1;
}

static int send_aap_response(int fd, Aap2__ResponseStatus status) {
  Aap2__AAPResponse resp = AAP2__AAPRESPONSE__INIT;
  resp.response_status = status;

  size_t packed_size = aap2__aapresponse__get_packed_size(&resp);
  uint8_t *buf = malloc(packed_size);
  aap2__aapresponse__pack(&resp, buf);

  int ret = send_varint(fd, packed_size);
  if (ret == 0) {
    ssize_t nsent = send(fd, buf, packed_size, 0);
    if (nsent != (ssize_t)packed_size)
      ret = -1;
  }
  free(buf);
  return ret;
}

static Aap2__AAPResponse *recv_aap_response(int fd) {
  uint64_t msg_len;
  if (recv_varint(fd, &msg_len) < 0)
    return NULL;
  uint8_t *buf = malloc(msg_len);
  if (recv_exact(fd, buf, msg_len) < 0) {
    free(buf);
    return NULL;
  }
  Aap2__AAPResponse *resp = aap2__aapresponse__unpack(NULL, msg_len, buf);
  free(buf);
  return resp;
}

aap2_client *connect_aap2(const char *path) {
  int socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  struct sockaddr_un addr;
  if (socket_fd < 0) {
    log_error("Failed to create socket");
    exit(1);
  }
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

  int ret = connect(socket_fd, (struct sockaddr *)&addr, sizeof(addr));
  if (ret < 0) {
    perror("Failed to connect to AAP2 socket");
    exit(1);
  }

  // Read the one-time marker byte sent by the server
  uint8_t marker;
  if (recv_exact(socket_fd, &marker, 1) < 0 || marker != 0x2f) {
    log_error("Failed to read server marker byte");
    exit(1);
  }

  // Read varint-prefixed AAPMessage
  uint64_t msg_len;
  if (recv_varint(socket_fd, &msg_len) < 0) {
    log_error("Failed to read message length");
    exit(1);
  }

  uint8_t *buf = malloc(msg_len);
  if (recv_exact(socket_fd, buf, msg_len) < 0) {
    log_error("Failed to read message body");
    free(buf);
    exit(1);
  }

  Aap2__AAPMessage *msg = aap2__aapmessage__unpack(NULL, msg_len, buf);
  free(buf);
  if (!msg) {
    log_error("Failed to unpack AAPMessage");
    exit(1);
  }
  if (msg->msg_case != AAP2__AAPMESSAGE__MSG_WELCOME || !msg->welcome) {
    log_error("Expected Welcome message, got type %d", msg->msg_case);
    aap2__aapmessage__free_unpacked(msg, NULL);
    exit(1);
  }

  aap2_client *client = malloc(sizeof(aap2_client));
  client->socket_fd = socket_fd;
  client->node_id = strdup(msg->welcome->node_id);
  aap2__aapmessage__free_unpacked(msg, NULL);

  log_info("Connected to AAP2 server, node ID: %s", client->node_id);
  return client;
}

int configure_aap2(aap2_client *client, int is_subscriber,
                   Aap2__AuthType auth_type, const char *secret,
                   const char *endpoint_id) {
  Aap2__ConnectionConfig config = AAP2__CONNECTION_CONFIG__INIT;
  config.is_subscriber = is_subscriber;
  config.auth_type = auth_type;
  config.secret = (char *)secret;
  config.endpoint_id = (char *)endpoint_id;

  Aap2__AAPMessage msg = AAP2__AAPMESSAGE__INIT;
  msg.msg_case = AAP2__AAPMESSAGE__MSG_CONFIG;
  msg.config = &config;

  size_t packed_size = aap2__aapmessage__get_packed_size(&msg);
  uint8_t *buf = malloc(packed_size);
  aap2__aapmessage__pack(&msg, buf);

  int ret = send_varint(client->socket_fd, packed_size);
  if (ret == 0) {
    ssize_t nsent = send(client->socket_fd, buf, packed_size, 0);
    if (nsent != (ssize_t)packed_size) {
      log_error("Failed to send config message");
      ret = -1;
    }
  }
  free(buf);
  if (ret < 0)
    return -1;

  // Read the server's AAPResponse to the config
  Aap2__AAPResponse *resp = recv_aap_response(client->socket_fd);
  if (!resp) {
    log_error("Failed to read config response");
    return -1;
  }
  if (resp->response_status != AAP2__RESPONSE_STATUS__RESPONSE_STATUS_SUCCESS) {
    log_error("Config rejected by server: status=%d", resp->response_status);
    aap2__aapresponse__free_unpacked(resp, NULL);
    return -1;
  }
  aap2__aapresponse__free_unpacked(resp, NULL);
  return 0;
}

int send_aap2(aap2_client *client, const char *dst_eid, const uint8_t *payload,
              size_t payload_len) {
  Aap2__BundleADU adu = AAP2__BUNDLE_ADU__INIT;
  adu.dst_eid = (char *)dst_eid;
  adu.payload_length = payload_len;

  Aap2__AAPMessage msg = AAP2__AAPMESSAGE__INIT;
  msg.msg_case = AAP2__AAPMESSAGE__MSG_ADU;
  msg.adu = &adu;

  size_t packed_size = aap2__aapmessage__get_packed_size(&msg);
  uint8_t *buf = malloc(packed_size);
  aap2__aapmessage__pack(&msg, buf);

  // Send varint length + protobuf message + raw payload
  int ret = send_varint(client->socket_fd, packed_size);
  if (ret == 0) {
    if (send(client->socket_fd, buf, packed_size, 0) != (ssize_t)packed_size)
      ret = -1;
  }
  free(buf);
  if (ret == 0) {
    if (send(client->socket_fd, payload, payload_len, 0) !=
        (ssize_t)payload_len)
      ret = -1;
  }
  if (ret < 0) {
    log_error("Failed to send ADU message");
    return -1;
  }

  // Read the AAPResponse — server returns bundle headers on success
  Aap2__AAPResponse *resp = recv_aap_response(client->socket_fd);
  if (!resp) {
    log_error("Failed to read ADU response");
    return -1;
  }
  if (resp->response_status != AAP2__RESPONSE_STATUS__RESPONSE_STATUS_SUCCESS) {
    log_error("ADU rejected by server: status=%d", resp->response_status);
    aap2__aapresponse__free_unpacked(resp, NULL);
    return -1;
  }
  aap2__aapresponse__free_unpacked(resp, NULL);
  return 0;
}

int close_aap2(aap2_client *client) {
  if (!client)
    return -1;
  close(client->socket_fd);
  free(client->node_id);
  free(client);
  return 0;
}

int listen_aap2(aap2_client *client) {
  for (;;) {
    uint64_t msg_len;
    if (recv_varint(client->socket_fd, &msg_len) < 0) {
      log_error("Failed to read message length");
      return -1;
    }

    uint8_t *buf = malloc(msg_len);
    if (recv_exact(client->socket_fd, buf, msg_len) < 0) {
      log_error("Failed to read message body");
      free(buf);
      return -1;
    }

    Aap2__AAPMessage *msg = aap2__aapmessage__unpack(NULL, msg_len, buf);
    free(buf);
    if (!msg) {
      log_error("Failed to unpack AAPMessage");
      return -1;
    }

    switch (msg->msg_case) {
    case AAP2__AAPMESSAGE__MSG_KEEPALIVE:
      // Must acknowledge every keepalive or the server will drop the connection
      if (send_aap_response(client->socket_fd,
                            AAP2__RESPONSE_STATUS__RESPONSE_STATUS_ACK) < 0) {
        log_error("Failed to send keepalive ACK");
        aap2__aapmessage__free_unpacked(msg, NULL);
        return -1;
      }
      break;

    case AAP2__AAPMESSAGE__MSG_ADU:
      if (msg->adu && msg->adu->payload_length > 0) {
        // Payload bytes follow the protobuf message on the wire
        uint8_t *payload = malloc(msg->adu->payload_length);
        if (recv_exact(client->socket_fd, payload, msg->adu->payload_length) <
            0) {
          log_error("Failed to read ADU payload");
          free(payload);
          aap2__aapmessage__free_unpacked(msg, NULL);
          return -1;
        }
		// must do stuff here
		log_info("Received ADU for EID %s, payload length %zu",
		 msg->adu->dst_eid, msg->adu->payload_length);
        free(payload);
      }
      // Acknowledge the ADU
      if (send_aap_response(client->socket_fd,
                            AAP2__RESPONSE_STATUS__RESPONSE_STATUS_ACK) < 0) {
        log_error("Failed to send ADU ACK");
        aap2__aapmessage__free_unpacked(msg, NULL);
        return -1;
      }
      break;

    default:
      log_warn("Received unexpected message type %d, ignoring", msg->msg_case);
      break;
    }

    aap2__aapmessage__free_unpacked(msg, NULL);
  }
  return 0;
}
