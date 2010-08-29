# @file    arch/x86_64/kernel/Makefile
# @author  Kato Takeshi
# @brief   カーネル本体の Makefile
#
# (C) 2010 Kato Takeshi.


ROOT = ../../..
MAKE_DEFAULTS ?= make.defaults
-include $(ROOT)/$(MAKE_DEFAULTS)

LZMA = lzma

CFLAGS += -fno-exceptions
CXXFLAGS += -fno-exceptions -fno-rtti
CPPFLAGS +=

KERNEL_DIR = ../../../kernel
KERNEL_OBJS = $(shell cat $(KERNEL_DIR)/objects.txt)

SRCS = entry.S exception_handler.S gnuc.c intr.cc kerninit.cc \
       physical_memmgr.cc \
       serialh.S serialout.cc setupdata.cc \
       vadr2padr.cc videoout.cc cpuinit.cc
OBJS := $(SRCS:%.cc=%.o)
OBJS := $(OBJS:%.S=%.o)
OBJS := $(OBJS:%.c=%.o)
OBJS += $(foreach x, $(KERNEL_OBJS), $(KERNEL_DIR)/$(x))

.PHONY: all
all : kernel.bin.lzma setup

.PHONY: setup
setup :
	$(MAKE) -C $@

kernel.bin.lzma : kernel.bin
	$(LZMA) -c $^ > $@

kernel.bin : $(OBJS)
	$(LD) -nostdlib -static -T kernel.ld $^ -o $@

%.o : %.S
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

%.o : %.cc
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

gnuc.o : gnuc.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	$(MAKE) -C setup $@
	$(RM) kernel.bin kernel.bin.lzma
	$(RM) $(OBJS)
	$(RM) Makefile.deps

.PHONY: depends
depends:
	$(MAKE) -C setup $@
	$(RM) Makefile.deps
	$(CPP) -MM $(CPPFLAGS) $(SRCS) >> Makefile.deps

-include Makefile.deps
