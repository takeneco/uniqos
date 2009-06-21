# FILE : arch/x86/kernel/Makefile
# VER  : 0.0.1
# LAST : 2009-06-21
# (C) Kato.T 2009

CFLAGS += -fno-exceptions -fno-rtti

OBJS = start32.o

all : kernel.bin

kernel.bin : $(OBJS)
	$(LD) -nostdlib -static -Ttext=0x100000 -T kernel.ld $^ -o $@

start32.o : start32.S
	$(CC) -c $(CPPFLAGS) $(CFLGAS) $< -o $@

clean:
	$(RM) kernel.bin
	$(RM) $(OBJS)

.PHONY: all clean

