# @file    arch/x86_64/kernel/setup/Makefile
# @version 0.0.0.2
# @author  Kato.T
# @brief   カーネル自身をセットアップするMakefile。

# (C) Kato.T 2010

CFLAGS += -fno-exceptions -fno-common -fno-rtti -Os
CXXFLAGS += -fno-exceptions -fno-common -fno-rtti -Os 
CPPFLAGS += -I../../../../include -I../../include -I../include

OLD_OBJS = setup16.o setup32.o
OBJS = a20.o memmgr.o memops.o \
 prekern.o setup.o termchain.o videoterm.o

all : setup.bin

setup.bin : $(OBJS)
	$(LD) -nostdlib -static -T setup.ld $^ -o $@

lzmadecwrap.o : lzmadecwrap.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -I../../../../external/C/lzma $< -o $@

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
