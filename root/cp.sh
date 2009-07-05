#!/bin/sh

mount -o loop arch/x86/fd1440.img /mnt/tmp2
cp arch/x86/root.bin /mnt/tmp2/ROOTCORE.BIN
umount /mnt/tmp2
