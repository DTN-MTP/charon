#include "aap2_client.h"
#include "log.h"
#include "proto/aap2.pb-c.h"
#include <net/if.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int recv_exact(int fd, void *buf, size_t len) {
  size_t total = 0;
  while (total < len) {
    ssize_t n = recv(fd, (uint8_t *)buf + total, len - total, 0);
    if (n <= 0)
      return -1;
    total += n;
  }

  return 0;
}

int send_exact(int fd, void *buf, size_t len) {
  size_t total = 0;
  while (total < len) {
    ssize_t n = send(fd, (uint8_t *)buf + total, len - total, 0);
    if (n <= 0)
      return -1;
    total += n;
  }
  return 0;
}

int recv_varint(int fd, uint64_t *out) {
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

int send_varint(int fd, uint64_t value) {
  uint8_t buf[10];
  int i = 0;
  
  do {
    buf[i++] = (uint8_t)(value & 0x7F);
    value >>= 7;
  } while (value != 0);
  
  for (int j = 0; j < i - 1; j++) {
    buf[j] |= 0x80;
  }
  
  return send_exact(fd, buf, i);
}

aap2info getaap2info(const char *aap2_url) {
  aap2info aap2_addr;
  memset(&aap2_addr, 0, sizeof(aap2_addr));

  char *url = strdup(aap2_url);
  if (!url) {
    log_error("OOM");
    exit(1);
  }

  char *prefix = strpbrk(url, ":");
  if (!prefix) {
    log_error("Invalid AAP2 address: '%s'", aap2_url);
    free(url);
    exit(1);
  }
  *prefix = '\0';
  prefix += 3;

  if (strcmp(url, "tcp") == 0) {
    aap2_addr.conn_type = AAP2_INET;
  } else if (strcmp(url, "unix") == 0) {
    aap2_addr.conn_type = AAP2_UNIX;
  } else {
    log_error("AAP2 address must start with 'tcp://' or 'unix://'");
    free(url);
    exit(1);
  }

  if (aap2_addr.conn_type == AAP2_UNIX) {
    aap2_addr.unix_path = prefix;
  } else {
    char *address = strdup(prefix);
    if (!address) {
      log_error("OOM");
      free(url);
      exit(1);
    }

    char *colon = strpbrk(address, ":");
    if (!colon) {
      log_error("AAP2 TCP address must be in format tcp://host:port");
      free(address);
      free(url);
      exit(1);
    }
    *colon = '\0';
    aap2_addr.host = address;
    aap2_addr.port = colon + 1;
  }

  return aap2_addr;
}

int create_unix_socket(const char *path) {
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0) {
    log_error("Couldn't initialize unix socket");
    close(fd);
    return -1;
  }

  struct sockaddr_un serv_addr;
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sun_family = AF_UNIX;
  strncpy(serv_addr.sun_path, path, sizeof(serv_addr.sun_path) - 1);

  uint8_t marker_byte;

  log_info("Connecting to %s", serv_addr.sun_path);
  if (connect(fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    log_error("Couldn't connect to unix socket");
    close(fd);
    return -1;
  }

  return fd;
}

int create_tcp_socket(const char *host, const char *port) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  int rv;
  if (fd < 0) {
    log_error("Couldn't initialize ip socket");
    close(fd);
    return -1;
  }

  struct addrinfo hints, *servinfo, *p;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
    log_error("Couldn't getaddrinfo : %s", gai_strerror(rv));
    close(fd);
    return -1;
  }

  if (connect(fd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
    log_error("Couldn't connect to ip socket");
    close(fd);
    return -1;
  }

  return fd;
}

char *receive_welcome_message(int fd) {
  uint8_t marker_byte;
  if (recv_exact(fd, &marker_byte, sizeof(marker_byte)) < 0) {
    log_error("Couldn't receive 0x2f bytes indicating aap version");
    return NULL;
  }

  if (marker_byte != 0x2f) {
    log_error("Didn't receive 0x2f");
    return NULL;
  }

  uint64_t msg_size;
  if (recv_varint(fd, &msg_size) < 0) {
    log_error("Couldn't receive var int");
    return NULL;
  }
  // msg_size++;

  log_info("Received everything %i", msg_size);

  uint8_t *message = malloc(msg_size);

  if (recv_exact(fd, message, msg_size) < 0) {
    log_error("Couldn't receive EID.");
    return NULL;
  }

  Aap2__AAPMessage *aap2_message =
      aap2__aapmessage__unpack(NULL, msg_size, message);
  free(message);

  if (aap2_message == NULL) {
    log_error("Failed to unpack message");
  }

  return aap2_message->welcome->node_id;
}

aap2_client *connect_aap2(const char *aap2_url, const char *secret_name) {
  aap2info infos = getaap2info(aap2_url);

  int socket_fd;

  if (infos.conn_type == AAP2_UNIX) {
    socket_fd = create_unix_socket(infos.unix_path);
  } else {
    socket_fd = create_tcp_socket(infos.host, infos.port);
  }

  char *node_eid = receive_welcome_message(socket_fd);

  if (node_eid == NULL) {
    exit(1);
  }

  aap2_client *client = calloc(1, sizeof(aap2_client));
  char *secret = getenv(secret_name);
  if (secret == NULL) {
    log_error("Couldn't retrieve AAP2 secret at this env variable : %s",
              secret_name);
    exit(1);
  }
  log_info(secret);

  client->infos = infos;
  client->node_eid = node_eid;
  client->socket_fd = socket_fd;
  client->secret = secret;

  return client;
}

// https://protobuf-c.github.io/protobuf-c/pack.html
int configure_aap2(aap2_client *client, int is_subscriber,
                   Aap2__AuthType auth_type, const char *secret,
                   const char *endpoint_id) {
  Aap2__ConnectionConfig config_message;
  aap2__connection_config__init(&config_message);
  config_message.endpoint_id = client->node_eid;
  config_message.auth_type = AAP2__AUTH_TYPE__AUTH_TYPE_DEFAULT;
  config_message.is_subscriber = is_subscriber;
  config_message.secret = client->secret;
  config_message.keepalive_seconds = 0;

  Aap2__AAPMessage wrapper;
  aap2__aapmessage__init(&wrapper);
  wrapper.msg_case = AAP2__AAPMESSAGE__MSG_CONFIG;
  wrapper.config = &config_message;

  size_t packed_size =
      aap2__aapmessage__get_packed_size(&wrapper);
  uint8_t *buf = malloc(packed_size);
  aap2__aapmessage__pack(&wrapper, buf);

  log_info("EID : %s", config_message.endpoint_id);
  log_info("secret : %s", config_message.secret);


  if(send_varint(client->socket_fd, packed_size) < 0) {
    log_error("Couldn't send varint");
    return -1;
  }

  if (send_exact(client->socket_fd, buf, packed_size) < 0) {
    log_error("Couldn't send configuration");
    return -1;
  }

  log_info("Configuration sent !");
  free(buf);

  uint64_t msg_size;
  if (recv_varint(client->socket_fd, &msg_size) < 0) {
    log_error("Couldn't receive var int");
    return -1;
  }
  // msg_size++;

  log_info("Received everything %i", msg_size);

  uint8_t *message = malloc(msg_size);

  if (recv_exact(client->socket_fd, message, msg_size) < 0) {
    log_error("Couldn't receive EID.");
    return -1;
  }

  Aap2__AAPResponse *aap2_response =
      aap2__aapresponse__unpack(NULL, msg_size, message);

  log_info("%i", aap2_response->response_status);

  free(message);

  return 0;
}

int send_aap2(aap2_client *client, const char *dst_eid, const uint8_t *payload,
              size_t payload_len) {
  return -1;
}

int close_aap2(aap2_client *client) {
  close(client->socket_fd);

  return 1;
}
