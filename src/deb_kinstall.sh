#!/bin/bash

# set -x  # print commands
set -e  # stop on exit

ARCHIVE=$1
VERSION=$2
TMP_DIR="./tmp"

if [ -z $ARCHIVE ] || [ -z $VERSION ]; then
    echo "Missing arg! kinstall ARCHIVE VERSION"
    exit 1
fi

# extract to tmp dir
echo "Extracting to $TMP_DIR ..."
mkdir -p $TMP_DIR
tar -xf "$ARCHIVE" -C $TMP_DIR

# copy kernel to /boot & modules to /lib/modules/
echo "Copy files to /boot & /lib/modules ..."
sudo cp "$TMP_DIR/boot/vmlinuz-$VERSION" "/boot/"
sudo cp "$TMP_DIR/boot/System.map-$VERSION" "/boot/"
sudo cp "$TMP_DIR/boot/config-$VERSION" "/boot/"
sudo cp -r "$TMP_DIR/lib/modules/$VERSION" "/lib/modules/"

# setup scripts
echo "Installing kernel..."
/etc/kernel/postinst.d/initramfs-tools $VERSION "/boot/vmlinuz-$VERSION"
/etc/kernel/postinst.d/unattended-upgrades $VERSION "/boot/vmlinuz-$VERSION"
/etc/kernel/postinst.d/zz-update-grub $VERSION "/boot/vmlinuz-$VERSION"

# cleanup
echo "Cleanup $TMP_DIR" 
rm -rf $TMP_DIR
