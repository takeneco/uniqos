# @author  KATO Takeshi
# @brief   カーネル本体の Makefile
#
# (C) 2010 KATO Takeshi


ROOT = ../../..
MAKE_DEFAULTS ?= make.defaults
-include $(ROOT)/$(MAKE_DEFAULTS)

LZMA = lzma

CFLAGS += -fno-exceptions
CXXFLAGS += -fno-exceptions -fno-rtti
CPPFLAGS +=

KERNEL_DIR = ../../../kernel
KERNEL_OBJS = $(shell cat $(KERNEL_DIR)/objects.txt)

ASM_SRCS = entry.S exception_handler.S serialh.S
C_SRCS = gnuc.c
CXX_SRCS = global_variables.cc \
	intr.cc kerninit.cc kernel_memory.cc physical_memory.cc \
	serialout.cc setupdata.cc \
	vadr2padr.cc videoout.cc cpuinit.cc
ASM_OBJS = $(ASM_SRCS:%.S=%.o)
C_OBJS = $(C_SRCS:%.c=%.o)
CXX_OBJS = $(CXX_SRCS:%.cc=%.o)
OBJS = $(ASM_OBJS) $(C_OBJS) $(CXX_OBJS)
OBJS += $(foreach x, $(KERNEL_OBJS), $(KERNEL_DIR)/$(x))

.PHONY: all
all : kernel.bin.lzma setup

.PHONY: setup
setup :
	$(MAKE) -C $@

kernel.bin.lzma : kernel.bin
	$(LZMA) -c $^ > $@

kernel.bin : $(OBJS)
	$(LD) -nostdlib -static -T kernel.ld $^ -o $@

%.o : %.S
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

%.o : %.cc
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

gnuc.o : gnuc.c
	$(CC) -c $(CPPFLAGS) $(CFLAGS) $< -o $@

.PHONY: clean
clean:
	$(MAKE) -C setup $@
	$(RM) kernel.bin kernel.bin.lzma
	$(RM) $(OBJS)
	$(RM) Makefile.deps

.PHONY: depends
depends:
	$(MAKE) -C setup $@
	$(RM) Makefile.deps
	$(CPP) -MM $(CPPFLAGS) $(SRCS) >> Makefile.deps

-include Makefile.deps
