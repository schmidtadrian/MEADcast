# Master's thesis: MEADcast
MEADcast is an IPv6-based sender-centric multicast protocol designed to
    facilitate multipoint communication in scenarios where IP Multicast is
    unavailable.
Our experiments demonstrated significant performance improvements with MEADcast
    deployment compared to IP Unicast, including a 56% reduction in network
    bandwidth utilization, an 81.23% decrease in sender upstream bandwidth
    consumption, and a 49.18% reduction in total transfer time.


## Key features
- Seamless transition from extensive unicast to multicast transmission
    (MEADcast).
- Encodes receivers and designated MEADcast routers in the MEADcast header
    (custom IPv6 routing header), which is processed by transient MEADcast
    routers.
- Receivers are agnostic to the usage of MEADcast (constantly receive IP
    unicast traffic).
- Endures across varying levels of network support, allowing for gradual
    deployment of MEADcast routers.
- Fallback to IP unicast if MEADcast is unavailable.
- Operates in two phases: discovery and data delivery phase.
- During the discovery phase the sender discovers MEADcast router along a path,
    and creates a topology tree.


## Router
The patch file [linux-6.5.y.patch](src/linux-6.5.y.patch) adds MEADcast router
    functionality to the linux kernel.
This feature can be enabled or disabled at compile-time using the
    `CONFIG_IPV6_MEADCAST` flag.
During runtime, MEADcast can be enabled or disabled by setting
    `/proc/sys/net/ipv6/meadcast/enable` to `1` or `0`.
The latest tested version is 6.5.3.

For information on how to build the router kernel image, see:
    [build_kernel.md](./doc/build_kernel.md).

Main tasks of the router includes:
- Forwarding of discovery requests and responding to them.
- Forwarding and replicating MEADcast data packets.
- Transforming MEADcast to IP Unicast packets.


## Sender
The sender software is supported on Linux and requires the Judy array library.
On debian-based systems install `libjudydebian1`.

Main tasks of the sender includes:
- Create a TUN device during startup, which represents a common interface to
    other applications.
- Send periodical discovery requests to all group members.
- Receive discovery responses and construct a topology tree based on the
    responses.
- Group receivers into MEADcast packets based on the topology tree.
- Transmit any data, sent to the TUN interface, to all group members via
    MEADcast or IP Unicast.

### Grouping
The grouping is based on the topology tree created during the discovery phase.
The sender is the root, MEADcast routers are intermediate nodes, and receivers
    are leaf nodes.
The sender performs a DFS search to find a MEADcast router with unvisited
    leaves.
These leaves are grouped into a new packet and more nearby leaves are added
    until the maximum packet size is reached.

There are several parameters to adjust the grouping algorithm.
For details please refer to the sender's help (`mdc_send --help`) or see
    [here](https://schmidtadrian.github.io/MEADcast/schm24.pdf#listing.caption.187).
For an in-depth description of the grouping algorithm and examples, refer to:
    [Grouping Algorithm](https://schmidtadrian.github.io/MEADcast/schm24.pdf#subsection.4.3.2)


## Testbed
For information on building the testbed, see:
    [mk_topo/README.md](./src/mk_topo/README.md).


## Further information
- Protocol specification:
    - [Protocol header](https://schmidtadrian.github.io/MEADcast/schm24.pdf#figure.caption.82)
    - [Field description](https://schmidtadrian.github.io/MEADcast/schm24.pdf#table.caption.83)
- Interaction
    - [Switch from discovery to data delivery phase](https://schmidtadrian.github.io/MEADcast/schm24.pdf#subsection.2.1.4)
    - [The path of a MEADcast packet](https://schmidtadrian.github.io/MEADcast/schm24.pdf#figure.caption.34)
- Experiment Details:
    - [Network Topology](https://schmidtadrian.github.io/MEADcast/schm24.pdf#subsection.4.4.1)
    - [Technical Infrastructure](https://schmidtadrian.github.io/MEADcast/schm24.pdf#subsection.4.4.2)
    - [Conduction](https://schmidtadrian.github.io/MEADcast/schm24.pdf#subsection.4.4.3)
- Evaluation:
    - [Results](https://schmidtadrian.github.io/MEADcast/schm24.pdf#section.5.1)
    - [Discussion](https://schmidtadrian.github.io/MEADcast/schm24.pdf#section.5.2)
- Publications:
    - [MEADcast paper](https://personales.upv.es/thinkmind/dl/journals/sec/sec_v12_n12_2019/sec_v12_n12_2019_2.pdf)
    - [Thesis](https://schmidtadrian.github.io/MEADcast/schm24.pdf)
