# @file    arch/x86_64/boot/bootldr/Makefile
# @version 0.0.0.1
# @author  Kato.T
# @brief   カーネル読み込みまでの処理をコンパイルする Makefile。

# (C) Kato.T 2010

BASEDIR=../../../..

CFLAGS += -Os -fno-exceptions -fno-rtti -m32
CXXFLAGS += -Os -fno-exceptions -fno-rtti -m32
CPPFLAGS += -I../include \
	-I../../../../include

OBJS = bios.o bootldr.o loadfat.o loadkern.o

all: bootldr.bin

bootldr.bin : $(OBJS)
	$(LD) -nostdlib -static -Ttext=0x7e00 -T bootldr.ld $^ -o $@

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -fno-common -nostdinc -nostdinc++ $< -o $@

%.o : %.S
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $^ -o $@

clean:
	$(RM) $(OBJS)
	$(RM) bootldr.bin

.PHONY: all clean

