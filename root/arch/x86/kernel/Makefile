# @file    arch/x86/kernel/Makefile
# @version 0.0.2
# @date    2009-07-26
# @author  Kato.T
# @brief   カーネル本体の Makefile
# (C) Kato.T 2009

LZMA = lzma

CFLAGS += -fno-exceptions -fno-rtti

OBJS = start32.o

all : kernel.bin.lzma

kernel.bin.lzma : kernel.bin
	$(LZMA) -c $^ > $@

kernel.bin : $(OBJS)
	$(LD) -nostdlib -static -Ttext=0x100000 -T kernel.ld $^ -o $@

start32.o : start32.S
	$(CC) -c $(CPPFLAGS) $(CFLGAS) $< -o $@

clean:
	$(RM) kernel.bin kernel.bin.lzma
	$(RM) $(OBJS)

.PHONY: all clean

