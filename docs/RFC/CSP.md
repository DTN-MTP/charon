# CubeSat Protocol (CSP) Tunneling

Charon currently only provides IP encapsulation thanks to a TUN virtual IP interface. But we also want to provide
encapsulation for CSP applications. We have two possibilities so far. Either encapsulate CAN frames or CSP packets.

I first wanted to go for the second solution : encapsulate CSP packets. Since having a "transport and quality of service" protocol
has the encapsulated layer is really common in VPNs it seemed like the right choice to make. Also [libcsp](https://github.com/libcsp/libcsp) supports an interface
making [csp tunneling pretty easy](https://libcsp.github.io/libcsp/tunnel.html) but the libcsp version that the CSUM uses (1.6) doesn't support
tunneling unfortunately.

Also developers would need to install a few new dependencies (and unfortunately some of these will have to be built by hand on a specific tag). 
Here is how I did on arch linux :

```bash
sudo pacman -S zeromq
yay -S libsocketcan-git
git clone --branch v1.6 --depth 1 https://github.com/libcsp/libcsp.git libcsp
cd libcsp
uv init
# change project version to python3.11
uv venv

python waf configure \
        --enable-can-socketcan \
        --enable-if-zmqhub \
        --enable-rdp \
        --with-os=posix \
    && python waf build install
```

So in the end, CSP encapsulation can not be the solution unless the CSUM is ok with using the latest version of libcsp.

# CAN encapsulation

A project named [cannelloni](https://github.com/mguentner/cannelloni/tree/master) already provides CAN encapsulation over Ethernet.
