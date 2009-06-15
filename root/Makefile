# FILE : Makefile
# VER  : 0.0.3
# LAST : 2009-05-17
# (C)  : T.Kato 2008-2009
#
# 全体をコンパイルする。

ARCH = arch/x86

PUTIMG = tools/putimg
MKFDIMG = tools/mkfdimg

CFLAGS += -pipe -Wall -W -fomit-frame-pointer

export CFLAGS

all : $(ARCH)/fd1440.img

# FAT12ヘッダの直後(0x3e)にブートセクタ bootsect.bin を挿入する。
# ブートセクタの直後に boot2.bin を挿入する。
$(ARCH)/fd1440.img : $(PUTIMG) $(MKFDIMG)
	$(MAKE) -C $(ARCH) fd1440.img

$(PUTIMG) $(MKFDIMG):
	$(MAKE) -C tools

clean:
	$(MAKE) -C $(ARCH)
	$(MAKE) -C tools clean

distclean: clean
	$(RM) fd1440.img

envecho:
	@echo CFLAGS=$(CFLAGS)
	@echo CPPFLAGS=$(CPPFLAGS)
	@echo MAKE=$(MAKE)
	@echo RM=$(RM)

.PHONY: all clean distclean envtest

