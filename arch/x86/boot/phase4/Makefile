# FILE : arch/x86/boot/phase4/Makefile
# VER  : 0.0.6
# LAST : 2009-07-22
# (C) Kato.T 2008-2009
#
# ３２ビット起動処理をコンパイルする Makefile

CFLAGS += -fno-exceptions -fno-common -Os -march=i486
CXXFLAGS = $(CFLAGS) -fno-rtti
CPPFLAGS += -I../../../../include -I../../include -I../include

OBJS = LzmaDec.o \
 comterm.o inthandler.o lzmadecwrap.o main.o memmgr.o setup32.o \
 termchain.o util.o videoterm.o

all : phase4.bin

phase4.bin : mvjmp.bin setup32.bin
	cat mvjmp.bin setup32.bin > phase4.bin

mvjmp.bin : mvjmp.o
	$(LD) -nostdlib -static -T mvjmp.ld $^ -o $@

setup32.bin : $(OBJS)
	$(LD) -nostdlib -static -Ttext=0x200000 -T setup32.ld $^ -o $@

lzmadecwrap.o : lzmadecwrap.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -I../../../../contrib/lzma/C $< -o $@

LzmaDec.o : ../../../../contrib/lzma/C/LzmaDec.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

%.o : %.S
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) mvjmp.bin setup32.bin phase4.bin
	$(RM) mvjmp.o $(OBJS)

.PHONY: all clean

