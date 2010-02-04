# @file    Makefile
# @version 0.0.0.4
# @brief   全体をコンパイルする。

# (C) T.Kato 2008-2010

ARCH = x86_64
#ARCH = x86

CC  = x86_64-pc-linux-gnu-gcc
CXX = x86_64-pc-linux-gnu-g++
AS  = x86_64-pc-linux-gnu-as
LD  = x86_64-pc-linux-gnu-ld
export CC CXX AS LD

CFLAGS = -pipe -Wall -W -fomit-frame-pointer
CXXFLAGS = -pipe -Wall -W -fomit-frame-pointer

CPPFLGS= \
	-I$(BASEDIR)/include \
	-I$(BASEDIR)/arch/$(ARCH)/include

CFLAGS_x86 = -DARCH_X86
CFLAGS_x86_64 = -DARCH_X86_64

CFLAGS += $(CFLAGS_$(ARCH))
CXXFLAGS += $(CFLAGS_$(ARCH))

export ARCH CFLAGS CXXFLAGS CPPFLAGS

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

