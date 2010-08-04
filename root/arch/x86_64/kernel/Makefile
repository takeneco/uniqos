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
KERNEL_OBJS := $(shell cat $(KERNEL_DIR)/objects.txt)

OBJS = entry.o exception_handler.o gnuc.o intr.o kerninit.o \
       serialh.o serialout.o setupdata.o \
       vadr2padr.o videoout.o cpuinit.o
OBJS += $(foreach x, $(KERNEL_OBJS), $(KERNEL_DIR)/$(x))

all : kernel.bin.lzma setup

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

clean:
	$(MAKE) -C setup $@
	$(RM) kernel.bin kernel.bin.lzma
	$(RM) $(OBJS)

.PHONY: all clean setup

