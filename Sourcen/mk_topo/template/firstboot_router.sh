#!/bin/bash

DISK="/dev/vda"
PART_NUM=1

# increase partition size
sed -e 's/\s*\([\+0-9a-zA-Z]*\).*/\1/' << EOF | fdisk $DISK
  d # del partition
  $PART_NUM # select partition 1
  n # new partition
    # default number
    # default start sector
    # default last sector
  w # write
EOF

resize2fs "$DISK$PART_NUM"


# enable ip forwarding
sed -i "/net\.ipv4\.ip_forward=1/s/^#//g" /etc/sysctl.conf
sed -i "/net\.ipv6\.conf.all.forwarding=1/s/^#//g" /etc/sysctl.conf
sysctl -p

# Install frr from https://deb.frrouting.org/ because frr from debian repos
# misses pim6d
# add GPG key
curl -s https://deb.frrouting.org/frr/keys.gpg \
    | sudo tee /usr/share/keyrings/frrouting.gpg > /dev/null

# possible values for FRRVER: frr-6 frr-7 frr-8 frr-9.0 frr-9.1 frr-stable
# frr-stable will be the latest official stable release
FRRVER="frr-stable"
echo deb '[signed-by=/usr/share/keyrings/frrouting.gpg]' https://deb.frrouting.org/frr \
     $(lsb_release -s -c) $FRRVER | sudo tee -a /etc/apt/sources.list.d/frr.list


# install packages
apt update -y
# Because we changed frr config before installation we need to tell apt how to
# handle this. see: https://askubuntu.com/questions/104899/make-apt-get-or-aptitude-run-with-y-but-not-prompt-for-replacement-of-configu
apt-get install frr tcpdump -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold"

# enable frr modules
sed -i "s/ospf6d=no/ospf6d=yes/" /etc/frr/daemons
sed -i "s/pim6d=no/pim6d=yes/" /etc/frr/daemons

systemctl restart frr
