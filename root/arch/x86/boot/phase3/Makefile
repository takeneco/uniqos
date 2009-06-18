# FILE : arch/x86/boot/phase3/Makefile
# VER  : 0.0.5
# LAST : 2009-06-18
# (C) Kato.T 2008-2009
#
# １６ビット起動処理をコンパイルする Makefile。

CFLAGS += -fno-exceptions -fno-rtti -march=i486 -Os
CPPFLAGS += -I../../../../include -I../include

OBJS = bios.o phase3.o setup16.o

all : phase3.bin

phase3.bin : $(OBJS)
	$(LD) -nostdlib -static -T phase3.ld $^ -o $@

phase3.o : phase3.S
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

setup16.o : setup16.cpp
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

bios.o : bios.S
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) phase3.bin 
	$(RM) $(OBJS)

.PHONY: all clean
