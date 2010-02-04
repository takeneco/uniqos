#!/bin/sh

mount -o loop arch/x86_64/fd1440.img /mnt/tmp2
cp arch/x86_64/root.bin /mnt/tmp2/ROOTCORE.BIN
umount /mnt/tmp2
