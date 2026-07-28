#include "config.h"
#include "ini.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>

static int handler(void *user, const char *section, const char *name,
                   const char *value) {
  charon_config *pconfig = (charon_config *)user;

  if (strcmp(section, "bundle") == 0) {
    if (strcmp(name, "aap2_address") == 0) {
      pconfig->aap2_address = strdup(value);
    } else if (strcmp(name, "remote_eid") == 0) {
      pconfig->remote_eid = strdup(value);
    } else if (strcmp(name, "secret_name") == 0) {
      pconfig->secret_name = strdup(value);
    } else {
      return 0;
    }
  } else if (strcmp(section, "ip") == 0) {
    if (strcmp(name, "address") == 0) {
      pconfig->address = strdup(value);
    } else if (strcmp(name, "mtu") == 0) {
      pconfig->mtu = atoi(value);
    } else {
      return 0; /* unknown name */
    }
  } else if (strcmp(section, "can") == 0) {
    if (strcmp(name, "bitrate") == 0) {
      pconfig->bitrate = atoi(value);
    } else {
      return 0;
    }
  } else if (strcmp(section, "interface") == 0) {
    if (strcmp(name, "type") == 0) {
      if (strcmp(value, "ip") == 0) {
        pconfig->interface_type = IP;
      } else if (strcmp(value, "can") == 0) {
        pconfig->interface_type = CAN;
      } else if (strcmp(value, "csp") == 0) {
        pconfig->interface_type = CSP;
      } else {
        return 0;
      }
    }
  } else if (strcmp(section, "csp") == 0) {
    if (strcmp(name, "port") == 0) {
      pconfig->csp_port = atoi(value);
    } else if (strcmp(name, "remote_address") == 0) {
      pconfig->peer_csp_address = atoi(value);
    } else if (strcmp(name, "agent_address") == 0) {
      pconfig->local_csp_address = atoi(value);
    } else if (strcmp(name, "role") == 0) {
      if (strcmp(value, "server") == 0) {
        pconfig->proxy_role = CHARON_LISTEN;
      } else {
        pconfig->proxy_role = CHARON_WRITE;
      }
    } else if (strcmp(name, "can_interface") == 0) {
      pconfig->can_interface = strdup(value);
    } else {
      return 0;
    }
  } else {
    return 0; /* unknown section */
  }
  return 1;
}

charon_config *read_config(const char *filename) {
  charon_config *config = calloc(1, sizeof(charon_config));

  if (ini_parse(filename, handler, config) < 0) {
    log_error("Failed to read config file '%s'", filename);
    free_config(config);
    return NULL;
  }
  if (config->interface_type == IP) {
    log_info("using IP mode with %s and MTU = %i", config->address,
             config->mtu);
  } else if (config->interface_type == CAN) {

    log_info("using CAN mode with %i bitrate", config->bitrate);
  }
  return config;
}

void free_config(charon_config *config) {
  if (!config)
    return;
  free(config->aap2_address);
  free(config->remote_eid);
  free(config->secret_name);
  free(config->address);
  free(config);
}
