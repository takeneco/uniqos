# FILE : arch/x86/boot/pahse1/Makefile
# VER  : 0.0.1
# LAST : 2009-05-17
# (C) Kato.T 2008-2009
#
# ブートセクタのMakefile

CFLAGS += -march=i486 -Os -fno-exceptions -fno-rtti
CPPFLAGS += -I../include

all: bootsect.bin

# ブートセクタは 0x7c00 にロードされ、
# プログラム自体は 0x7c00+0x3e から始まる。
bootsect.bin: bootsect.o
	$(CC) -nostdlib -static -Ttext=0x7c3e -T bootsect.ld $^ -o $@

bootsect.o : bootsect.S
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $^ -o $@

clean:
	$(RM) bootsect.o bootsect.bin

.PHONY: all clean

