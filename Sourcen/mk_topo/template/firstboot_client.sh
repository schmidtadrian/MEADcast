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

apt-get update -y
apt-get install ssmping iperf ncat libjudydebian1 libatomic1 -y

