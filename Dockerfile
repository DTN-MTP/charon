FROM debian:trixie-slim
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    git \
    wget \
    unzip \
    ca-certificates \
    libprotobuf-c-dev \
    protobuf-c-compiler \
	iproute2 \
    && rm -rf /var/lib/apt/lists/*

RUN wget https://github.com/protocolbuffers/protobuf/releases/download/v34.1/protoc-34.1-linux-x86_64.zip && \
    unzip protoc-34.1-linux-x86_64.zip -d /usr/local/ && \
    rm protoc-34.1-linux-x86_64.zip

WORKDIR /app
COPY . .
RUN mkdir build && make proto && make

CMD ["/bin/bash"]
