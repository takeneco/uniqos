#!/bin/sh

mount -o loop arch/x86/fd1440.img /mnt/tmp
cp arch/x86/boot/rootboot.bin /mnt/tmp/ROOTCORE.BIN
umount /mnt/tmp
