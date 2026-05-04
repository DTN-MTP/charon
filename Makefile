CC      = gcc
CFLAGS  = -Wall -Wextra -g -O2 \
	-Isrc \
	-DLOG_USE_COLOR \
	-g
LDFLAGS = -lprotobuf-c

SRC = src/*.c \
	  src/proto/*.c
	
BIN = build/charon

.PHONY: all clean dev clean-dev proto

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f build/*

proto:
	protoc proto/aap2.proto --c_out=./src
