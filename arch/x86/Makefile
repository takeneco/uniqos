# FILE : arch/x86/Makefile
# VER  : 0.0.3
# LAST : 2009-06-21
# (C) Kato.T 2008-2009
#
# x86 の起動ディスクイメージを作る Makefile

PUTIMG = ../../tools/putimg
MKFDIMG = ../../tools/mkfdimg

CFLAGS +=

export CFLAGS

ROOT = root.bin
PHASE1 = boot/phase1/bootsect.bin
PHASE2 = boot/phase2/phase2.bin
PHASE3 = boot/phase3/phase3.bin
PHASE4 = boot/phase4/phase4.bin
KERNEL = kernel/kernel.bin.lzma

all : fd1440.img

# FAT12ヘッダの直後(0x3e)にブートセクタ bootsect.bin を挿入する。
# ブートセクタの直後に phase2.bin を挿入する。
# bootsect.bin と phase2.bin のために先頭６セクタを予約領域とする。
fd1440.img : KERNEL
	$(MKFDIMG) -r 6 $@
	$(PUTIMG) -t 0x3e $(PHASE1) $@
	$(PUTIMG) -t 0x0200 $(PHASE2) $@

BOOT :
	$(MAKE) -C boot

KERNEL : BOOT
	$(MAKE) -C kernel
	cat $(PHASE3) $(PHASE4) $(KERNEL) > $(ROOT)

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

