# @file    arch/x86_64/boot/bootsect/Makefile
# @author  Kato Takeshi
# @brief   ブートセクタのMakefile
#
# (C) 2010 Kato Takeshi

CFLAGS += -Os -fno-exceptions
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

