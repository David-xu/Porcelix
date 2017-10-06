#!/bin/bash
QEMU=/home/x10168/src_build/build/qemu-2.8.0/x86_64-softmmu/qemu-system-x86_64

${QEMU} --help > help.txt
${QEMU} Use -device ? >> help.txt

