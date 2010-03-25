# @file    kernel/Makefile
# @author  Kato Takeshi
# @brief   CPU not depends kernel part Makefile.
#
# (C) 2008-2010 Kato Takeshi


ROOT = ..
MAKE_DEFAULTS ?= make.defaults
include $(ROOT)/$(MAKE_DEFAULTS)

CFLAGS += -fno-exceptions -O3
CXXFLAGS += -fno-exceptions -O3 -fno-rtti
CPPFLAGS += -I../include \
	-I../arch/$(ARCH)/include

OBJLIST = objects.txt
OBJS = fileif.o kout.o string.o string.o
# init.o

all : $(OBJS)
	echo $(OBJS) > $(OBJLIST)

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

