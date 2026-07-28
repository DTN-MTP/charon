// Integration tests for the CSP server

#include "../src/config.h"
#include "../src/log.h"
#include "../src/proxy.h"
#include "helpers.h"
#include "test.h"
#include <stdlib.h>

charon_config config_for_test() {
  charon_config config = {.aap2_address = "tcp://0.0.0.0:4226",
                          .remote_eid = "dtn://bob.dtn/charon",
                          .secret_name = "D3TN_AAP2_KEY",
                          .interface_type = CSP,
                          .csp_port = 10,
                          .can_interface = "vcan0",
                          .proxy_role = CHARON_LISTEN,
                          .peer_csp_address = 2,
                          .local_csp_address = 1};
  putenv("D3TN_AAP2_KEY=my_extremely_secret_secret_omg_i_love_this_secretly_"
         "secret_secret");
  return config;
}

int test_can_interface_exists() {
  charon_config config = config_for_test();
  int interface_status = assert_can_interface_exists(config.can_interface);
  ASSERT_TRUE(interface_status >= 0);
  TEST_PASS();
  return 1;
}

int test_can_interface_not_exists() {
  charon_config config = config_for_test();
  int interface_status = assert_can_interface_exists("randomstuff");
  ASSERT_TRUE(interface_status < 0);
  TEST_PASS();
  return 1;
}

int test_should_init_proxy() {
  charon_proxy proxy;
  charon_config config = config_for_test();

  int config_status = charon_proxy_init(&proxy, &config);
  ASSERT_EQ(config_status, 1);
  TEST_PASS();
  return 1;
}

int test_should_answer_as_server() {
  charon_proxy proxy;
  charon_config config = config_for_test();
  charon_proxy_init(&proxy, &config);

}

int main() {
  log_warn("This test suite needs the ud3tn containers to be running.\nTo do "
           "so, just "
           "run : \ndocker compose up");
  if (test_can_interface_exists() < 0) {
    TEST_FAIL("Interface vcan0 doesn't exist");
  }

  if (test_can_interface_not_exists() < 0) {
    TEST_FAIL("Interface randomstuff exist but shouldn't");
  }

  if (test_should_init_proxy() < 0) {
    TEST_FAIL("Couldn't initialize proxy");
  }

  return 0;
}
