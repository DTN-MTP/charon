#include "../src/aap2_client.h"
#include "../src/log.h"
#include "test.h"
#include <stdlib.h>
#include <string.h>

int test_aap2_client_send_message_correctly() {
  putenv("D3TN_AAP2_KEY=my_extremely_secret_secret_omg_i_love_this_secretly_"
         "secret_secret");
  aap2_client *client = connect_aap2("tcp://0.0.0.0:4225", "D3TN_AAP2_KEY");

  if (configure_aap2(client, 0, 0) < 0) {
    TEST_FAIL("Failed to configure client");
  }

  if (send_aap2(client, "dtn://bob/test", (uint8_t *)"hello", strlen("hello")) <
      0) {
    TEST_FAIL("Failed to send first message");
  }

  TEST_PASS();
  return 1;
}

int main() {
  if (test_aap2_client_send_message_correctly() < 0) {
    return 1;
  }

  return 0;
}
