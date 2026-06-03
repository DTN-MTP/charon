# Charon 🚣

A tunneling protocol through the bundle protocole enabling Delay/Disruption Tolerant Networking (DTN) for applications that weren't. Firstly implemented for over space-grade radio links through CSP.

## Build from source (Linux)

You will need `protobuf-c-compiler` and `libprotobuf-c-dev` to compile the project.

```bash
make proto
make
```

The binary will be available in the `build` folder.

## Configuration

The configuration file is located in `charon.conf`. It is a INI file with the following structure : 

```ini
[bundle]
aap2_socket=./aap2.sock ;; The path to the AAP2 Unix socket.
remote_eid=dtn://charon.dtn/ ;; The EID of the remote node to send bundles to.
secret_name=D3TN_AAP2_KEY ;; The name of the environment variable to connect to the AAP2 socket with.

[interface]
address=10.0.0.1/32 ;; The IP address of the interface to create. It must be a /32 address.
mtu=1500 ;; The maximum transmitable unit of the tunnel
```

## Quick-start guide

Before getting started you need to set-up a bidirectional DTN network between two µD3TN nodes. To do so, you can either [follow their getting started guide](https://d3tn.gitlab.io/ud3tn/posix_quick_start_guide/). Or use this docker compose : 

```bash
docker compose --profile tests up --build
```

Then in one terminal run : 

```bash
docker exec -it bob nc -l -p 4000
```

And in another terminal run : 

```bash
docker exec -it alice nc 10.0.0.2 4000
```
