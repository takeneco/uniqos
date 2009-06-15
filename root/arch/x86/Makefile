# FILE : arch/x86/Makefile
# VER  : 0.0.2
# LAST : 2009-05-17
# (C) Kato.T 2008-2009
#
# x86 のディスクイメージを作る Makefile

PUTIMG = ../../tools/putimg
MKFDIMG = ../../tools/mkfdimg

CFLAGS +=

export CFLAGS

PHASE1 = boot/phase1/bootsect.bin
PHASE2 = boot/phase2/phase2.bin

all : fd1440.img

# FAT12ヘッダの直後(0x3e)にブートセクタ bootsect.bin を挿入する。
# ブートセクタの直後に phase2.bin を挿入する。
# bootsect.bin と phase2.bin のために先頭６セクタを予約領域とする。
fd1440.img : $(PHASE1) $(PHASE2)
	$(MKFDIMG) -r 6 $@
	$(PUTIMG) -t 0x3e $(PHASE1) $@
	$(PUTIMG) -t 0x0200 $(PHASE2) $@

$(PHASE1) $(PHASE2) :
	$(MAKE) -C boot

clean:
	$(MAKE) -C boot clean

distclean: clean
	$(RM) fd1440.img

envecho:
	@echo CFLAGS=$(CFLAGS)
	@echo CPPFLAGS=$(CPPFLAGS)
	@echo MAKE=$(MAKE)
	@echo RM=$(RM)

.PHONY: all clean distclean envecho

