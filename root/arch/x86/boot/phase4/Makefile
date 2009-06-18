# FILE : arch/x86/boot/phase4/Makefile
# VER  : 0.0.4
# LAST : 2009-05-31
# (C)  : Kato.T 2008-2009
#
# ３２ビット起動処理をコンパイルする Makefile

CFLAGS += -fno-exceptions -fno-rtti
CPPFLAGS += -I../../../../include -I../../include -I../include

all : phase4.bin

phase4.bin : movejump.bin setup32.bin
	cat movejump.bin setup32.bin > phase4.bin

movejump.bin : movejump.o
	$(LD) -nostdlib -static -T movejump.ld $^ -o $@

movejump.o : movejump.S
	$(CC) -c $(CPPFLAGS) $(CFLGAS) -march=i486 $< -o $@

setup32.bin: inthandler.o main.o setup32.o
	$(LD) -nostdlib -static -Ttext=0x200000 -T setup32.ld $^ -o $@

setup32.o: setup32.S
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -march=i486 -Os $< -o $@

inthandler.o: inthandler.S
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -march=i486 -Os $< -o $@

main.o: main.cpp
	$(CXX) -c $(CPPFLAGS) $(CFLAGS) -fno-common -nostdinc -nostdinc++ $< -o $@

clean:
	$(RM) movejump.bin setup32.bin phase4.bin
	$(RM) movejump.o inthandler.o setup32.o main.o

.PHONY: all clean

%.o : %.S
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $^ -o $@
