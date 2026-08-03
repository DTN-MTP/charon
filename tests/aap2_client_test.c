#include "../src/aap2_client.h"
#include "../src/log.h"
#include "test.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>

void handler(aap2_answer *answer, void *params) {}

int test_aap2_client_schedules_message_correctly() {
  putenv("D3TN_AAP2_KEY=my_extremely_secret_secret_omg_i_love_this_secretly_"
         "secret_secret");
  aap2_client client;

  aap2_init_client(&client, &handler);

  connect_aap2(&client, "tcp://0.0.0.0:4225", "D3TN_AAP2_KEY");

  if (configure_aap2(&client, 0, 0) < 0) {
    TEST_FAIL("Failed to configure client");
  }

  send_aap2(&client, "dtn://bob.dtn/test", (uint8_t *)"hello", strlen("hello"));

  while (client.running) {
    struct pollfd pfd = {
        .fd = client.socket_fd,
        .events = POLLIN,
    };

    if (client.tx_head)
      pfd.events |= POLLOUT;

    int ret = poll(&pfd, 1, 100);
    if (ret < 0) {
      log_error("poll");
      break;
    }

    if (pfd.revents & POLLIN) {
    }

    if (pfd.revents & POLLOUT) {
      aap2_message *msg = client.tx_head;
      if (msg) {
        close(client.socket_fd);
        client.running = 0;
        log_info("got message");
        TEST_PASS();
        return 1;
      }
    }
  }

  close(client.socket_fd);
  TEST_FAIL("Shouldn't have stopped");
  return 1;
}


int test_aap2_client_writes_adu_correctly() {
  putenv("D3TN_AAP2_KEY=my_extremely_secret_secret_omg_i_love_this_secretly_"
         "secret_secret");
  aap2_client client;

  aap2_init_client(&client, &handler);

  connect_aap2(&client, "tcp://0.0.0.0:4225", "D3TN_AAP2_KEY");

  if (configure_aap2(&client, 0, 0) < 0) {
    TEST_FAIL("Failed to configure client");
  }

  aap2_message msg = {.dst_eid = "dtn://bob.dtn/",
                      .next = NULL,
                      .payload = (uint8_t *)"hello",
                      .payload_len = strlen("hello")};

  if (write_aap2(&client, &msg) < 0) {
    TEST_FAIL("Couldn't write message");
  }
  TEST_PASS();
  return -1;
}

int main() {
  if (test_aap2_client_schedules_message_correctly() < 0) {
    return 1;
  }

  if (test_aap2_client_writes_adu_correctly() < 0) {
    return 1;
  }

  return 0;
}
