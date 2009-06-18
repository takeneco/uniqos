# FILE : arch/x86/boot/phase2/Makefile
# VER  : 0.0.2
# LAST : 2009-05-17
# (C) Kato.T 2008-2009
#
# カーネル読み込みまでの処理をコンパイルする Makefile

CFLAGS += -march=i486 -Os -fno-exceptions -fno-rtti
CPPFLAGS += -I../../../../include -I../include

OBJS = bios.o loadfat.o loadkern.o phase2.o

all: phase2.bin

phase2.bin : $(OBJS)
	$(LD) -nostdlib -static -Ttext=0x7e00 -T phase2.ld $^ -o $@

phase2.o : phase2.S
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $^ -o $@

bios.o : bios.S
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $^ -o $@

loadkern.o : loadkern.cpp
	$(CXX) -c $(CPPFLAGS) $(CFLAGS) -fno-common -nostdinc -nostdinc++ $< -o $@

loadfat.o: loadfat.cpp
	$(CXX) -c $(CPPFLAGS) $(CFLAGS) -fno-common -nostdinc -nostdinc++ $< -o $@

clean:
	$(RM) $(OBJS)
	$(RM) phase2.bin

.PHONY: all clean

