FROM python:3.10-slim-bookworm AS builder
RUN apt update && apt install -y \
		git \
		build-essential \
		pkg-config \
		wget \
		libsocketcan-dev \
		libprotobuf-c-dev \
		protobuf-c-compiler \
        ca-certificates \
        unzip

RUN wget https://github.com/protocolbuffers/protobuf/releases/download/v34.1/protoc-34.1-linux-x86_64.zip && \
    unzip protoc-34.1-linux-x86_64.zip -d /usr/local/ && \
    rm protoc-34.1-linux-x86_64.zip

RUN git clone --depth 1 --branch v1.6 https://github.com/libcsp/libcsp
RUN cd libcsp && ./waf configure --enable-can-socketcan && ./waf && cd ..

RUN mkdir build
COPY src src
COPY Makefile Makefile
RUN make CSP_REPO_DIR=./libcsp

FROM debian:trixie-slim


RUN apt-get update && apt-get install -y --no-install-recommends \
	iproute2 \
	libprotobuf-c-dev \
	libsocketcan-dev \
	netcat-traditional \
	can-utils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder ./build/charon /app/charon

CMD ["/bin/bash"]
