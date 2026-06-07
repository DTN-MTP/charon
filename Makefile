CC      = gcc
CFLAGS  = -Wall -Wextra -g -O2 \
	-Isrc \
	-DLOG_USE_COLOR \
	-g
LDFLAGS = -lprotobuf-c -lpthread

SRC = src/*.c \
	  src/proto/*.c


TEST_SRC = $(filter-out src/main.c, $(wildcard src/*.c)) \
      $(wildcard src/proto/*.c)

TEST_DIR  = tests
BUILD_DIR = build

test_%: $(BUILD_DIR)/test_%
	./$(BUILD_DIR)/test_$*

BIN = build/charon

.PHONY: all clean dev clean-dev proto test

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f build/*

proto:
	protoc proto/aap2.proto --c_out=./src

test: $(BUILD_DIR)/test_$(TEST)
	./$(BUILD_DIR)/test_$(TEST)

$(BUILD_DIR)/test_%: $(TEST_DIR)/%_test.c $(TEST_SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

