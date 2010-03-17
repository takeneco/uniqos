# @file    arch/x86_64/kernel/setup/Makefile
# @author  Kato Takeshi
# @brief   カーネル自身をセットアップするMakefile。
#
# (C) 2010 Kato Takeshi.


ROOT = ../../../..
MAKE_DEFAULTS ?= make.defaults
include $(ROOT)/$(MAKE_DEFAULTS)

CFLAGS += -fno-exceptions -Os
CXXFLAGS += -fno-exceptions -fno-rtti -Os 
CPPFLAGS += -I../include

OBJS = LzmaDec.o a20.o lzmadecwrap.o memmgr.o memops.o \
 prekern.o setup.o termchain.o videoterm.o


all : setup.bin

setup.bin : $(OBJS)
	$(LD) -nostdlib -static -T setup.ld $^ -o $@

lzmadecwrap.o : lzmadecwrap.cc
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -I$(ROOT)/external/lzma/C $< -o $@

LzmaDec.o : ../../../../external/lzma/C/LzmaDec.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

%.o : %.cc
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

%.o : %.S
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) setup.bin
	$(RM) $(OBJS)

.PHONY: all clean
