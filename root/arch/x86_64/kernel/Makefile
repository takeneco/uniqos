# @file    arch/x86_64/kernel/Makefile
# @version 0.0.0.1
# @author  Kato.T
# @brief   カーネル本体の Makefile
#
# (C) Kato.T 2010


-include ../../../$(MAKE_DEFAULTS)

LZMA = lzma

CFLAGS += -fno-exceptions -O3
CPPFLAGS = -I../../../include

KERNEL_DIR = ../../../kernel
KERNEL_OBJS := $(shell cat $(KERNEL_DIR)/objects.txt)

OBJS = entry.o ktty_x86.o 
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

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

%.o : %.cc
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

clean:
	$(MAKE) -C setup $@
	$(RM) kernel.bin kernel.bin.lzma
	$(RM) $(OBJS)

.PHONY: all clean setup

