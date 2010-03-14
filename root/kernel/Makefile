# @file    kernel/Makefile
# @author  Kato Takeshi
# @brief   CPU not depends kernel part Makefile.
#
# (C) Kato.T 2008-2010


ROOT = ..
include $(ROOT)/$(MAKE_DEFAULTS)

CFLAGS += -fno-exceptions -fno-common
CXXFLAGS = $(CFLAGS) -fno-rtti
CPPFLAGS += -I../include \
	-I../arch/$(ARCH)/include

OBJLIST = objects.txt
OBJS = kout.o
#OBJS = init.o

all : $(OBJS)
	echo $(OBJS) > objects.txt

lzmadecwrap.o : lzmadecwrap.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -I../contrib/lzma/C $< -o $@

LzmaDec.o : ../contrib/lzma/C/LzmaDec.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

%.o : %.cc
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

%.o : %.S
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(OBJLIST)
	$(RM) $(OBJS)

.PHONY: all clean

