# mk_topo
Script to generate network topology.

## Prerequisites
Python yaml parser:
```sh
pip install PyYAML
```

Create a static mgm net beforehand (`mgm.xml`):
```
<network>
  <name>mgm</name>
  <bridge name='br_mgm' stp='on' delay='0'/>
  <ip address='10.10.0.1' netmask='255.255.0.0'></ip>
</network>
```

Define and start mgm net:
```sh
virsh net-define mgm.xml
virsh net-autostart mgm.xml
virsh net-start mgm.xml
```

## Overview
Based on KVM, [nocloud image](https://cloud.debian.org/images/cloud/) (netplan), libguestfs-tools, frr.
Network is IPv6 only, expect IPv4 mgm net.
Routers use ospf6 and pim-sm for dynamic routing.

To use direct kernel boot ensure, all required modules are directly included
in the kernel itself (no external module).

## Example
```sh
sudo python3 mk_topo.py \
-t template/topo4.json \
-o out/ \
-n template/net.xml \
-u user \
--ssh ~/id_rsa_vms.pub \
--client-run template/run.sh template/run_client.sh \
--router-run template/run.sh template/run_router.sh \
--backing-file /var/lib/libvirt/images/templates/debian-12-nocloud-amd64.raw \
-c template/chrony.conf \
-f template/frr.conf \
-k kernel=/var/lib/libvirt/images/kernel/vmlinuz-6.5.2-dirty,initrd=/var/lib/libvirt/images/kernel/initrd.img-6.5.2-dirty,kernel_args="root=/dev/vda1 ro console=ttyS0,115200 2"
```

## Testing
To test whether IPv6 Multicast routing is working use ssmping and iperf:

### ssmping
Multicast sender:
```sh
ssmpingd
```

Receivers:
```sh
ssmping -6 -I <interface> <sender_ip>
```

### iperf
Multicast sender:
(Setting TTL with `-T` is mandantory)
```sh
iperf -c [ff3e::4321:1234]%enp2s0 -V -u -i 1 -T 32 -t 0
```

Receivers:
(Specifing source with `-H` is mandantory)
```sh
iperf -s -B [ff3e::4321:1234]%enp2s0 -V -u -i 1 -H fd14::b:11

```
