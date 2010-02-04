# @file    arch/x86_64/boot/bootsect/Makefile
# @version 0.0.1
# @author  Kato.T
# @brief   ブートセクタのMakefile

# (C) Kato.T 2010

CFLAGS += -Os -fno-exceptions -fno-rtti
CPPFLAGS += -I../include

all: bootsect.bin

# ブートセクタは 0x7c00 にロードされ、
# プログラム自体は 0x7c00+0x3e から始まる。
bootsect.bin: bootsect.o
	$(LD) -nostdlib -static -Ttext=0x7c3e -T bootsect.ld $^ -o $@

bootsect.o : bootsect.S
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $^ -o $@

clean:
	$(RM) bootsect.o bootsect.bin

.PHONY: all clean

