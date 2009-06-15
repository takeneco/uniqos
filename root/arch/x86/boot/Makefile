# FILE : arch/x86/boot/Makefile
# VER  : 0.0.5
# LAST : 2009-06-15
# (C) Kato.T 2008-2009
#
# x86固有のブート処理をコンパイルする Makefile

all : phases rootboot.bin

phases :
	$(MAKE) -C phase1
	$(MAKE) -C phase2
	$(MAKE) -C phase3
	$(MAKE) -C phase4

rootboot.bin : phase3/phase3.bin phase4/phase4.bin
	cat phase3/phase3.bin phase4/phase4.bin > $@

clean :
	$(MAKE) -C phase1 clean
	$(MAKE) -C phase2 clean
	$(MAKE) -C phase3 clean
	$(MAKE) -C phase4 clean
	$(RM) rootboot.bin

.PHONY: all phases clean
