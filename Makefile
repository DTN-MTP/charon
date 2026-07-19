CC      = gcc
CFLAGS  = -Wall -Wextra -g -O2 \
	-Isrc \
	-DLOG_USE_COLOR \
	-g
LDFLAGS = -lprotobuf-c -lpthread

# ============================================================================
# libcsp integration - simplified
# ============================================================================
# Usage: make CSP_REPO_DIR=/path/to/libcsp

CSP_REPO_DIR ?= $(error CSP_REPO_DIR not set. Pass: make CSP_REPO_DIR=/path/to/libcsp)

CFLAGS += -I$(CSP_REPO_DIR)/include -I$(CSP_REPO_DIR)/build/include
LDFLAGS += -L$(CSP_REPO_DIR)/build -lcsp -lsocketcan -lpthread -lrt

# ============================================================================
# Build configuration
# ============================================================================

SRC = src/*.c \
	  src/proto/*.c

TEST_SRC = $(filter-out src/main.c, $(wildcard src/*.c)) \
      $(wildcard src/proto/*.c)

TEST_DIR  = tests
BUILD_DIR = build

test_%: $(BUILD_DIR)/test_%
	./$(BUILD_DIR)/test_$*

BIN = build/charon

.PHONY: all clean dev clean-dev proto test help

all: $(BIN)

# Help target
help:
	@echo "Makefile targets:"
	@echo "  make                           Build binary ($(BIN))"
	@echo "  make proto                     Generate protobuf code"
	@echo "  make test TEST=<name>          Run specific test"
	@echo "  make clean                     Remove build artifacts"
	@echo "  make help                      Show this help"
	@echo ""
	@echo "libcsp options:"
	@echo "  make CSP_REPO_DIR=/path/to/libcsp      Use system libcsp"
	@echo ""
	@echo "Example:"
	@echo "  make CSP_REPO_DIR=~/libcsp"

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
