#!/bin/bash

#DISK="/dev/vda" # --firstboot
DISK="/dev/sda" # --run
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


apt-get update -y
# Because we create chrony config before installation, force existing config
apt-get install chrony -y -o Dpkg::Options::="--force-confdef" -o Dpkg::Options::="--force-confold"

