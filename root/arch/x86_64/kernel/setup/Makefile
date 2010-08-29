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
CPPFLAGS += -I$(ROOT)/external/lzma/C

SRCS = a20.S lzmadecwrap.cc memmgr.cc memops.cc \
 prekern.cc setup.S termchain.cc videoterm.cc
OBJS = $(SRCS:%.cc=%.o)
OBJS := $(OBJS:%.S=%.o)
OBJS += LzmaDec.o

.PHONY: all
all : setup.bin

setup.bin : $(OBJS)
	$(LD) -nostdlib -static -T setup.ld $^ -o $@

lzmadecwrap.o : lzmadecwrap.cc
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

LzmaDec.o : ../../../../external/lzma/C/LzmaDec.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

%.o : %.cc
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

%.o : %.S
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

.PHONY: depends
depends:
	$(RM) Makefile.deps
	$(CPP) -MM $(CPPFLAGS) $(SRCS) >> Makefile.deps

.PHONY: clean
clean:
	$(RM) setup.bin
	$(RM) $(OBJS)
	$(RM) Makefile.deps
