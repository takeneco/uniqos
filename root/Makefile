# @file    Makefile
# @version 0.0.4.0
# @date    2009-08-02
# (C) T.Kato 2008-2009
#
# 全体をコンパイルする。

ARCH = x86

CFLAGS = -pipe -Wall -W -fomit-frame-pointer

CPPFLGS= \
	-I$(BASEDIR)/include \
	-I$(BASEDIR)/arch/$(ARCH)/include

CFLAGS_x86 = -DARCH_X86

CFLAGS += $(CFLAGS_$(ARCH))

export ARCH CFLAGS

all : fd1440

# FAT12ヘッダの直後(0x3e)にブートセクタ bootsect.bin を挿入する。
# ブートセクタの直後に boot2.bin を挿入する。
fd1440 :
	$(MAKE) -C tools
	$(MAKE) -C kernel
	$(MAKE) -C arch/$(ARCH)

clean:
	$(MAKE) -C arch/$(ARCH) $@
	$(MAKE) -C kernel $@
	$(MAKE) -C tools $@

distclean: clean
	$(RM) fd1440.img

envecho:
	@echo CFLAGS=$(CFLAGS)
	@echo CPPFLAGS=$(CPPFLAGS)
	@echo MAKE=$(MAKE)
	@echo RM=$(RM)

.PHONY: all clean distclean envtest fd1440

