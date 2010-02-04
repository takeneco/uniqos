# @file    arch/x86_64/boot/phase4/Makefile
# @version 0.0.1
# @brief   64ビット起動処理をコンパイルする Makefile

# (C) Kato.T 2008-2010

CFLAGS += -fno-exceptions -fno-common -Os
CPPFLAGS += -I../../../../include -I../../include -I../include

OBJS = LzmaDec.o \
 comterm.o inthandler.o lzmadecwrap.o main.o memmgr.o setup32.o \
 termchain.o util.o videoterm.o

all : phase4.bin

phase4.bin : mvjmp.bin setup32.bin
	cat mvjmp.bin setup32.bin > phase4.bin

mvjmp.bin : mvjmp.o
	$(LD) -nostdlib -static -T mvjmp.ld $^ -o $@

lzmadecwrap.o : lzmadecwrap.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -I../../../../contrib/lzma/C $< -o $@

LzmaDec.o : ../../../../contrib/lzma/C/LzmaDec.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

clean:
	$(RM) mvjmp.bin setup32.bin phase4.bin
	$(RM) mvjmp.o $(OBJS)

.PHONY: all clean

