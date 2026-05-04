#include "proto/aap2.pb-c.h"

enum CONNECTION_TYPE {
  AAP2_INET,
  AAP2_UNIX
};


typedef struct {
  char *unix_path;
  char *host;
  char *port;
  enum CONNECTION_TYPE conn_type;
} aap2info;

typedef struct {
  int socket_fd;
  char *node_eid;
  aap2info infos;
} aap2_client;


typedef void (*aap2_message_handler)(
    Aap2__AAPMessage); // define handler for aap2 message

aap2_client* connect_aap2(const char *path);
int configure_aap2(aap2_client *client, int is_subscriber,
                   Aap2__AuthType auth_type, const char *secret,
                   const char *endpoint_id);

int send_aap2(aap2_client *client, const char *dst_eid, const uint8_t *payload,
              size_t payload_len);
int close_aap2(aap2_client *client);
int listen_aap2(aap2_client *client, aap2_message_handler);
