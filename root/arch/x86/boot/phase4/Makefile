# FILE : arch/x86/boot/phase4/Makefile
# VER  : 0.0.5
# LAST : 2009-06-21
# (C) Kato.T 2008-2009
#
# ３２ビット起動処理をコンパイルする Makefile

CFLAGS += -fno-exceptions -fno-rtti
CXXFLAGS = $(CFLAGS)
CPPFLAGS += -I../../../../include -I../../include -I../include

all : phase4.bin

phase4.bin : mvjmp.bin setup32.bin
	cat mvjmp.bin setup32.bin > phase4.bin

mvjmp.bin : mvjmp.o
	$(LD) -nostdlib -static -T mvjmp.ld $^ -o $@

mvjmp.o : mvjmp.S
	$(CC) -c $(CPPFLAGS) $(CFLGAS) -march=i486 $< -o $@

setup32.bin : comterm.o console.o inthandler.o main.o memmgr.o setup32.o util.o
	$(LD) -nostdlib -static -Ttext=0x200000 -T setup32.ld $^ -o $@

setup32.o : setup32.S
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -march=i486 -Os $< -o $@

inthandler.o : inthandler.S
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -march=i486 -Os $< -o $@

main.o : main.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -fno-common $< -o $@

memmgr.o : memmgr.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -fno-common $< -o $@

console.o : console.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -fno-common $< -o $@

util.o : util.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -fno-common $< -o $@

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -fno-common $< -o $@

clean:
	$(RM) mvjmp.bin setup32.bin phase4.bin
	$(RM) mvjmp.o comterm.o console.o inthandler.o main.o memmgr.o setup32.o util.o

.PHONY: all clean

