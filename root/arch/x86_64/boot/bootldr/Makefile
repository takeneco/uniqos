# @file    arch/x86_64/boot/bootldr/Makefile
# @author  Kato Takeshi
# @brief   カーネル読み込みまでの処理をコンパイルする Makefile。
#
# (C) 2010 Kato Takeshi.

ROOT = ../../../..
MAKE_DEFAULTS ?= make.defaults
-include $(ROOT)/$(MAKE_DEFAULTS)

CFLAGS = -pipe -Wall -W -fomit-frame-pointer -Os -fno-exceptions -m32
CXXFLAGS = -pipe -Wall -W -fomit-frame-pointer -Os -fno-exceptions -fno-rtti -m32
CPPFLAGS = -DARCH_W32 -DARCH_LE -I../include -I$(ROOT)/include

OBJS = tr.o bootldr.o loadfat.o loadkern.o


all: bootldr.bin

bootldr.bin : $(OBJS)
	$(LD) -nostdlib -static -Ttext=0x7e00 -T bootldr.ld $^ -o $@

%.o : %.cc
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -fno-common $< -o $@

%.o : %.S
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $^ -o $@

clean:
	$(RM) $(OBJS)
	$(RM) bootldr.bin

.PHONY: all clean

