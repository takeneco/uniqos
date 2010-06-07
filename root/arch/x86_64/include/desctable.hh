// @file   arch/x86_64/include/desctable.hh
// @author Kato Takeshi
// @brief  Descripter table.
//
// (C) 2010 Kato Takeshi.

#ifndef _ARCH_X86_64_INCLUDE_DESCTABLE_HH_
#define _ARCH_X86_64_INCLUDE_DESCTABLE_HH_

enum {
	GDT_KERN_CODESEG = 1,
	GDT_KERN_DATASEG = 2,
	GDT_USER_CODESEG = 3,
	GDT_USER_DATASEG = 4,
};


#endif  // Include guard.
