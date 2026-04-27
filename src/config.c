#include "config.h"
#include "ini.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>

static int handler(void *user, const char *section, const char *name,
                   const char *value) {
  charon_config *pconfig = (charon_config *)user;

  if (strcmp(section, "bundle") == 0) {
    if (strcmp(name, "aap2_socket") == 0) {
      pconfig->aap2_socket = strdup(value);
    } else if (strcmp(name, "remote_eid") == 0) {
      pconfig->remote_eid = strdup(value);
    } else if (strcmp(name, "secret_name") == 0) {
      pconfig->secret_name = strdup(value);
    } else if (strcmp(section, "interface") == 0) {
      if (strcmp(name, "address") == 0) {
        pconfig->address = strdup(value);
      } else if (strcmp(name, "mtu") == 0) {
        pconfig->mtu = atoi(value);
      } else {
        return 0; /* unknown name */
      }
    }
  } else {
    return 0; /* unknown section */
  }
  return 1;
}

charon_config *read_config(const char *filename) {
  charon_config *config = malloc(sizeof(charon_config));
  if (ini_parse(filename, handler, config) < 0) {
    log_error("Failed to read config file '%s'", filename);
    free(config);
    return NULL;
  }
  return config;
}
