# @file    arch/x86/kernel/Makefile
# @version 0.0.2
# @date    2009-07-26
# @author  Kato.T
# @brief   カーネル本体の Makefile
#
# (C) Kato.T 2009

LZMA = lzma

CFLAGS += -fno-exceptions -fno-rtti -O3 \
	-I../../../include
CPPFLAGS = $(CFLAGS)

KERNEL_DIR = ../../../kernel
KERNEL_OBJS := $(shell cat $(KERNEL_DIR)/objects.txt)

OBJS = ktty_x86.o start32.o
OBJS += $(foreach x, $(KERNEL_OBJS), $(KERNEL_DIR)/$(x))

all : kernel.bin.lzma

kernel.bin.lzma : kernel.bin
	$(LZMA) -c $^ > $@

kernel.bin : $(OBJS)
	$(LD) -nostdlib -static -T kernel.ld $^ -o $@

start32.o : start32.S
	$(CC) -c $(CPPFLAGS) $(CFLGAS) $< -o $@

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

clean:
	$(RM) kernel.bin kernel.bin.lzma
	$(RM) $(OBJS)

.PHONY: all clean

