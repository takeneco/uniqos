# @file    arch/x86_64/boot/phase3/Makefile
# @version 0.0.0.1
# @author  Kato.T
# @brief   ３２ビット起動処理をコンパイルする Makefile。

# (C) Kato.T 2008-2009

CFLAGS += -fno-exceptions -fno-common -fno-rtti -Os
CXXFLAGS += -fno-exceptions -fno-common -fno-rtti -Os 
CPPFLAGS += -I../../../../include -I../include

OLD_OBJS = bios.o setup16.o setup32.o
OBJS = a20.o setup.o

all : setup.bin

setup.bin : $(OBJS)
	$(LD) -nostdlib -static -Ttext=0x0000 -T setup.ld $^ -o $@

setup16.o : setup16.cpp
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

%.o : %.S
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) setup.bin
	$(RM) $(OBJS)

.PHONY: all clean
