#!/bin/bash

# enable kvm ptp
modprobe ptp_kvm
echo ptp_kvm > /etc/modules-load.d/ptp_kvm.conf

apt-get install ssmping iperf ncat libjudydebian1 libatomic1 -y

