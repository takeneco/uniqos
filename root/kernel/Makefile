# @file    kernel/Makefile
# @version 0.0.1
# @author  Kato.T
# @brief   カーネルの Makefile

# (C) Kato.T 2008-2010

BASEDIR=..

CFLAGS += -fno-exceptions -fno-common
CXXFLAGS = $(CFLAGS) -fno-rtti
CPPFLAGS += -I../include \
	-I../arch/$(ARCH)/include

OBJLIST = objects.txt
OBJS = init.o ktty.o

all : $(OBJS)
	echo $(OBJS) > objects.txt

lzmadecwrap.o : lzmadecwrap.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -I../contrib/lzma/C $< -o $@

LzmaDec.o : ../contrib/lzma/C/LzmaDec.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

%.o : %.S
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

clean:
	$(RM) $(OBJLIST)
	$(RM) $(OBJS)

.PHONY: all clean

