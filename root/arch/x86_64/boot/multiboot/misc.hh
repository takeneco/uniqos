/// @file   misc.hh
//
// (C) 2011 KATO Takeshi
//

#ifndef ARCH_X86_64_BOOT_MULTIBOOT_MISC_HH_
#define ARCH_X86_64_BOOT_MULTIBOOT_MISC_HH_

#include "easy_alloc2.hh"


typedef easy_alloc<256> allocator;
typedef easy_separator<allocator> separator;

enum MEM_SLOT {
	MEM_NORMAL = 0,

	// カーネルへ jmp した直後にカーネルが使用可能なヒープを BOOTHEAP と
	// 呼ぶことにする。
	// メモリ管理上は BOOTHEAP を区別し、可能な限り空いたままにしておく。
	MEM_BOOTHEAP = 1,
};

void  init_alloc();
allocator* get_alloc();

cause::stype pre_load(u32 magic, const u32* tag);
cause::stype post_load(u32* tag);


#endif  // include guard
