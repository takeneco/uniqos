# FILE :tools/ Makefile
# VER  : 0.0.2
# LAST : 2009-06-15
# (C) Kato.T 2008-2009
#
# ツール一式のMakefile

#CFLAGS += 

CPPFLAGS += -I../include

LDFLAGS += -lstdc++

all : mkfdimg putimg

mkfdimg : mkfdimg.o cmdparse.o
putimg : putimg.o cmdparse.o

clean : cleanobj
	$(RM) mkfdimg putimg

cleanobj :
	$(RM) mkfdimg.o putimg.o cmdparse.o
	$(MAKE) -C ut clean

.PHONY : all clean cleanobj

#.SUFFIXES:
#.SUFFIXES: .c .cpp .o
