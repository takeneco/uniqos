# @file    arch/x86_64/boot/Makefile
# @version 0.0.0.1
# @author  Kato.T
# @brief   x86_64固有のブート処理をコンパイルする Makefile

# (C) Kato.T 2010

all : phases

phases :
	$(MAKE) -C bootsect
	$(MAKE) -C bootldr
	$(MAKE) -C setup

clean :
	$(MAKE) -C bootsect clean
	$(MAKE) -C bootldr clean
	$(MAKE) -C setup clean

.PHONY: all phases clean
