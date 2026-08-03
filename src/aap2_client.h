#ifndef AAP2_CLIENT_H
#define AAP2_CLIENT_H

#include "proto/aap2.pb-c.h"
#include <bits/pthreadtypes.h>
#include <stdint.h>

enum CONNECTION_TYPE { AAP2_INET, AAP2_UNIX };

typedef struct {
  Aap2__AAPMessage *message;
  char *payload;
} aap2_answer;

typedef struct {
  char *unix_path;
  char *host;
  char *port;
  enum CONNECTION_TYPE conn_type;
} aap2info;

typedef void (*aap2_message_handler)(aap2_answer *,
                                     void *); // define handler for aap2 message
typedef struct aap2_message {
  uint8_t *payload;
  size_t payload_len;
  char* dst_eid;
  struct aap2_message *next;
} aap2_message;

typedef struct {
  char *node_eid;
  aap2info infos;
  char *secret;
  int socket_fd;         // Socket file descriptor
  int running;           // Thread control flag
  pthread_t thread;      // I/O thread
  pthread_mutex_t lock;  // Protects TX queue
  pthread_cond_t cond;   // Signals new TX data
  aap2_message *tx_head; // TX queue (linked list)
  aap2_message *tx_tail;
  aap2_message_handler on_recv;
  void *recv_arg; // User data for callback
} aap2_client;

void aap2_init_client(aap2_client *client, aap2_message_handler handler);
void connect_aap2(aap2_client *client, const char *path,
                  const char *secret_name);
int configure_aap2(aap2_client *client, int is_subscriber,
                   Aap2__AuthType auth_type);

void send_aap2(aap2_client *client, const char *dst_eid, uint8_t *payload, size_t payload_len);
int close_aap2(aap2_client *client);
int recv_aap2(aap2_client *client, void *rx);

int recv_exact(int fd, void *buf, size_t len);

int recv_varint(int fd, uint64_t *out);
int send_varint(int fd, uint64_t value);

int recv_one_adu(aap2_answer *answer, aap2_client *client);
int handle_aap2_response(uint8_t *message, uint64_t msg_size);

int write_aap2(aap2_client *client, aap2_message *message);

#endif // AAP2_CLIENT_H
