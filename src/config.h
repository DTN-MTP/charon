#ifndef CONFIG_H
#define CONFIG_H

enum CHARON_INTERFACE_TYPE {
		IP,
		CAN,
		CSP
};

enum CHARON_PROXY_ROLE {
		CHARON_LISTEN,
		CHARON_WRITE
};

typedef struct {
  enum CHARON_INTERFACE_TYPE interface_type;
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
  // csp
  char *can_interface;
  int csp_port;
  int peer_csp_address;
  int local_csp_address;
  enum CHARON_PROXY_ROLE proxy_role;
} charon_config;

charon_config *read_config(const char *filename);
void free_config(charon_config *config);

#endif

