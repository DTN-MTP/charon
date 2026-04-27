typedef struct {
  char *aap2_socket;
  char *remote_eid;
  char *address;
  char *secret_name;
  int mtu;
} charon_config;

charon_config *read_config(const char *filename);
