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

.PHONY: all
all : fd1440.img

# FAT12ヘッダの直後(0x3e)にブートセクタ bootsect.bin を挿入する。
# ブートセクタの直後に bootldr.bin を挿入する。
# bootsect.bin と bootldr.bin のために先頭７セクタを予約領域とする。
fd1440.img : KERNEL
	$(MKFDIMG) -r 7 $@
	$(PUTIMG) -t 0x3e $(BOOTSECT) $@
	$(PUTIMG) -t 0x0200 $(BOOTLDR) $@
	mcopy -i $@ $(ROOT) ::ROOTCORE.BIN

.PHONY: BOOT
BOOT:
	$(MAKE) -C boot

.PHONY: KERNEL
KERNEL: BOOT
	$(MAKE) -C kernel
	cat $(SETUP) $(KERNEL) > $(ROOT)

.PHONY: depends
depends:
	$(MAKE) -C kernel $@

.PHONY: clean
clean:
	$(MAKE) -C boot clean
	$(MAKE) -C kernel clean
	$(RM) $(ROOT) fd1440.img

.PHONY: envecho
envecho:
	@echo CFLAGS=$(CFLAGS)
	@echo CPPFLAGS=$(CPPFLAGS)
	@echo MAKE=$(MAKE)
	@echo RM=$(RM)


