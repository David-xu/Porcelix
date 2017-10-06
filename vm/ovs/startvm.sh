#!/bin/bash
QEMU=/home/x10168/src_build/build/qemu-2.8.0/x86_64-softmmu/qemu-system-x86_64

# -drive if=virtio,file=hd.img,cache=none
# -enable-kvm
# -cdrom ubuntu-12.04.5-server-i386.iso

${QEMU} \
-m 512 -enable-kvm \
-cdrom ../../mld.iso				\
-boot menu=on				\
-netdev tap,id=mynet0,ifname=tap0,script=tapup.sh,downscript=tapdown.sh,macaddr=0x12:0x34:0x56:0x78:0x90:0x01	\
-device rtl8139,netdev=mynet0

#${QEMU} \
#-m 512 \
#-kernel bzImage \
#-gdb tcp::1234 -S

#${QEMU} -m 512				\
#	-cdrom mld.iso					\
#	-serial stdio					\
#	-net nic,macaddr=0x12:0x34:0x56:0x78:0x90:0xab	\
#	-net tap,id=mynet0,ifname=tap0,script=tapup.sh,downscritp=tapdown.sh	\
#        -device rtl8139,netdev=mynet0

