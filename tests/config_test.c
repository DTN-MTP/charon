#include "../src/config.h"
#include "test.h"
#include <string.h>

int test_config_parsed() {
  char *config_file_path = "./tests/configs/working.conf";

  charon_config *conf = read_config(config_file_path);

  char *aap2_address = conf->aap2_address;
  char *remote_eid = conf->remote_eid;
  char *secret_name = conf->secret_name;

  char *address = conf->address;
  int mtu = conf->mtu;

  ASSERT_TRUE(strcmp(aap2_address, "tcp://0.0.0.0:4225") == 0);
  ASSERT_TRUE(strcmp(remote_eid, "dtn://bob.dtn/hey") == 0);
  ASSERT_TRUE(strcmp(secret_name, "D3TN_AAP2_KEY") == 0);
  ASSERT_TRUE(strcmp(address, "10.0.0.1") == 0);
  ASSERT_EQ(mtu, 1500);

  free_config(conf);
  TEST_PASS();
  return 0;
}

int main() {
  if (test_config_parsed() > 0) {
    return 1;
  }
  return 0;
}
