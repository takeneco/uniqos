# @file    arch/x86_64/Makefile
# @version 0.0.0.1
# @author  Kato.T
# @brief   x86_64 の起動ディスクイメージを作る Makefile

# (C) Kato.T 2010

PUTIMG = ../../tools/putimg
MKFDIMG = ../../tools/mkfdimg

CFLAGS +=

export CFLAGS

ROOT = root.bin
BOOTSECT = boot/bootsect/bootsect.bin
BOOTLDR  = boot/bootldr/bootldr.bin
SETUP = kernel/setup/setup.bin
#PHASE4 = boot/phase4/phase4.bin
KERNEL = kernel/kernel.bin.lzma

all : fd1440.img

# FAT12ヘッダの直後(0x3e)にブートセクタ bootsect.bin を挿入する。
# ブートセクタの直後に phase2.bin を挿入する。
# bootsect.bin と phase2.bin のために先頭６セクタを予約領域とする。
fd1440.img : KERNEL
	$(MKFDIMG) -r 7 $@
	$(PUTIMG) -t 0x3e $(BOOTSECT) $@
	$(PUTIMG) -t 0x0200 $(BOOTLDR) $@
	mcopy -i $@ $(ROOT) ::ROOTCORE.BIN

BOOT :
	$(MAKE) -C boot

KERNEL : BOOT
	$(MAKE) -C kernel
	#cat $(SETUP) $(KERNEL) > $(ROOT)
	cat $(SETUP) kernel/kernel.bin > $(ROOT)

clean:
	$(MAKE) -C boot clean
	$(MAKE) -C kernel clean
	$(RM) $(ROOT) fd1440.img

envecho:
	@echo CFLAGS=$(CFLAGS)
	@echo CPPFLAGS=$(CPPFLAGS)
	@echo MAKE=$(MAKE)
	@echo RM=$(RM)

.PHONY: all clean envecho BOOT KERNEL

