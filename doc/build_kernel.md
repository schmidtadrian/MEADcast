# Build MEADcast Kernel
Build Kernel image and initrd for `mk_topo` script.

## Get Source
If you haven't downloaded the kernel source already, the quickest way is to
just download the required tag:
```sh
git clone --depth 1 --branch linux-6.5.y git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git
```

## Apply patch
Apply the [MEADcast patch file](../src/linux-6.5.y.patch) to the kernel source
    by moving into the corresponding directory and running:
```sh
patch -p1 < ../linux-6.5.y.patch
```

## Get Kernel Config
Start a VM based on the NoCloud image.

Run the following command inside the VM to get all loaded kernel modules.
Save the `lsmod_target` file somewhere outside the VM and shut it down.
```sh
lsmod > lsmod_target
```

Next, specify this file while creating the `.config` file inside the kernel source directory:
```sh
yes "" | make LSMOD="<lsmod_target>" localmodconfig
```

Enable the config params for running the kernel as a KVM guest:
```sh
make kvm_guest.config
```

Search for the `# DOS/FAT/EXFAT/NT Filesystems` section in the `.config` file
and set the `CONFIG_FAT_FS` and `CONFIG_VFAT_FS` to `y`. It should look something
like this:
```sh
#
# DOS/FAT/EXFAT/NT Filesystems
#
CONFIG_FAT_FS=y
# CONFIG_MSDOS_FS is not set
CONFIG_VFAT_FS=y
```

## Build the Kernel
Since, we are building the kernel for a different target machine, run the
following command. If successful, it should create a file with
a name similar to `linux-6.5.2-dirty-x86_64.tar.gz`.
```sh
make ARCH=x86_64 CROSS_COMPILE=x86_64-linux-gnu- targz-pkg -j<NUM_CORE>
```

## Install the Kernel
Start the NoCloud image VM again and copy the file
`linux-6.5.2-dirty-x86_64.tar.gz` into it along with the `deb_kinstall.sh` file.

To install the Kernel in the VM run ([deb_kinstall.sh](../src/deb_kinstall.sh)):
```sh
./deb_kinstall.sh linux-6.5.2-dirty-x86_64.tar.gz 6.5.2-dirty
```

Afterwards reboot the VM and verify that the new Kernel is loaded by running:
```sh
uname -a
```
The output should contain `dirty`, and the proc filesystem should contain a
MEADcast directory at `/proc/sys/net/ipv6/meadcast`.

## Get VMlinuz and initrd file
If everything worked as expected, copy the freshly build initrd and kernel out
of the VM onto the outer host system (for your command copy them into
`/var/lib/libvirt/images/kernel/`). The files should be the following:
```sh
/boot/initrd.img-6.5.2-dirty
/boot/vmlinuz-6.5.2-dirty
```

Now you can build the topology.

