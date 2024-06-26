# Master's thesis: MEADcast
MEADcast is an IPv6-based sender-centric multicast protocol designed to
    facilitate multipoint communication in scenarios where IP Multicast is
    unavailable.
For further information see:
    [Thesis](https://schmidtadrian.github.io/MEADcast/schm24.pdf)

## Key features
- Seamless transition from extensive unicast to multicast transmission
    (MEADcast)
- Encodes receivers and designated MEADcast routers in the MEADcast header
    (custom IPv6 routing header), which is processed by transient MEADcast
    routers
- Receivers are agnostic to the usage of MEADcast (constantly receive IP
    unicast traffic)
- Endures across varying levels of network support, allowing for gradual
    deployment of MEADcast routers
- Fallback to IP unicast if MEADcast is not available
- Operates in two phases: discovery and data delivery phase
- During the discovery phase the sender discovers MEADcast router along a path

## Source
This repo contains both MEADcast sender and router functionality.

### Sender
The sender software is supported on Linux and requires the Judy array library.
On debian-based systems install `libjudydebian1`.

### Router
The patch file [linux-6.5.y.patch](src/linux-6.5.y.patch) adds MEADcast router
    functionality to the linux kernel.
The latest tested version is 6.5.3.
For information on how to build the router kernel image, see:
    [build_kernel.md](./doc/build_kernel.md).

## Testbed
For information on how to build the testbed, see:
    [mk_topo/README.md](./src/mk_topo/README.md).
