# @file    arch/x86_64/kernel/Makefile
# @version 0.0.0.1
# @author  Kato.T
# @brief   カーネル本体の Makefile
#
# (C) Kato.T 2010

LZMA = lzma

CFLAGS += -fno-exceptions -fno-rtti -O3 \
	-I../../../include
CPPFLAGS = $(CFLAGS)

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

clean:
	$(MAKE) -C setup $@
	$(RM) kernel.bin kernel.bin.lzma
	$(RM) $(OBJS)

.PHONY: all clean setup

