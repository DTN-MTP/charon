typedef struct {
  // bundle
  char *aap2_address;
  char *aap2_socket;
  char *remote_eid;
  // ip
  char *address;
  char *secret_name;
  int mtu;
  // can
  int bitrate;
} charon_config;

charon_config *read_config(const char *filename);
void free_config(charon_config *config);
