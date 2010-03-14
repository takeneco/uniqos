# @file    arch/x86_64/kernel/setup/Makefile
# @author  Kato Takeshi
# @brief   カーネル自身をセットアップするMakefile。
#
# (C) Kato Takeshi 2010

CFLAGS += -fno-exceptions -fno-common -Os
CXXFLAGS += -fno-exceptions -fno-common -fno-rtti -Os 
CPPFLAGS += -I../../../../include -I../../include -I../include

OBJS = LzmaDec.o a20.o lzmadecwrap.o memmgr.o memops.o \
 prekern.o setup.o termchain.o videoterm.o

all : setup.bin

setup.bin : $(OBJS)
	$(LD) -nostdlib -static -T setup.ld $^ -o $@

lzmadecwrap.o : lzmadecwrap.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -I../../../../external/lzma/C $< -o $@

LzmaDec.o : ../../../../external/lzma/C/LzmaDec.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

%.o : %.S
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) setup.bin
	$(RM) $(OBJS)

.PHONY: all clean
