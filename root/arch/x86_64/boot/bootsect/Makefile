# @author  KATO Takeshi
# @brief   ブートセクタのMakefile
#
# (C) 2010 KATO Takeshi

CFLAGS = -m64 -pipe -Wall -W -fomit-frame-pointer -Os -fno-exceptions
CPPFLAGS = -I../include

ASM_SRCS = bootsect.S
ASM_OBJS = $(ASM_SRCS:%.S=%.o)
OBJS = $(ASM_OBJS)

.PHONY: all
all: bootsect.bin

# ブートセクタは 0x7c00 にロードされ、
# プログラム自体は 0x7c00+0x3e から始まる。
bootsect.bin: $(OBJS)
	$(LD) -nostdlib -static -Ttext=0x7c3e -T bootsect.ld $^ -o $@

%.o : %.S
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

.PHONY: depends
depends:
	$(RM) Makefile.deps
	$(CPP) -MM $(CPPFLAGS) $(SRCS) >> Makefile.deps

.PHONY: clean
clean:
	$(RM) $(OBJS) bootsect.bin

-include Makefile.deps
