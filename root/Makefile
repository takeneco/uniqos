# @file    Makefile
# @author  Kato Takeshi
#
# (C) 2008-2010 Kato Takeshi.

export MAKE_DEFAULTS = make.defaults
include $(MAKE_DEFAULTS)

export CC CXX AS LD

all : fd1440

fd1440 :
	$(MAKE) -C tools
	$(MAKE) -C kernel
	$(MAKE) -C arch/$(ARCH)

clean:
	$(MAKE) -C arch/$(ARCH) $@
	$(MAKE) -C kernel $@

distclean: clean
	$(RM) fd1440.img

cleanall: distclean
	$(MAKE) -C tools clean

envecho:
	@echo CFLAGS=$(CFLAGS)
	@echo CPPFLAGS=$(CPPFLAGS)
	@echo MAKE=$(MAKE)
	@echo RM=$(RM)

.PHONY: all clean cleanall distclean envecho fd1440

