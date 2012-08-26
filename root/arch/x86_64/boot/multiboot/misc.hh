/// @file   misc.hh
//
// (C) 2011-2012 KATO Takeshi
//

#ifndef ARCH_X86_64_BOOT_MULTIBOOT_MISC_HH_
#define ARCH_X86_64_BOOT_MULTIBOOT_MISC_HH_

#include <cheap_alloc.hh>
#include <log_target.hh>


typedef cheap_alloc<256> allocator;
typedef cheap_alloc_separator<allocator> separator;

enum MEM_SLOT {
	SLOT_INDEX_NORMAL = 0,

	// カーネルへ jmp した直後からカーネルがメモリをセットアップするまで
	// の間に RAM として使用可能なヒープを BOOTHEAP と呼ぶことにする。
	// メモリ管理上は BOOTHEAP を区別し、可能な限り空いたままにしておく。
	SLOT_INDEX_BOOTHEAP = 1,

	SLOT_INDEX_CONVENTIONAL = 2,
};
enum MEM_SLOT_MASK {
	SLOTM_NORMAL       = 1 << SLOT_INDEX_NORMAL,
	SLOTM_BOOTHEAP     = 1 << SLOT_INDEX_BOOTHEAP,
	SLOTM_CONVENTIONAL = 1 << SLOT_INDEX_CONVENTIONAL,
};

void  init_alloc();
allocator* get_alloc();

cause::type pre_load(u32 magic, const u32* tag);
cause::type post_load(u32* tag);

extern struct load_info_
{
	u64 entry_adr;
	u64 page_table_adr;

	u64 bootinfo_adr;
} load_info;


// log

class log : public log_target
{
	DISALLOW_COPY_AND_ASSIGN(log);

public:
	log(int i = 0);
	~log();
};

void log_set(uint i, file* target);


// memlog_file

class memlog_file : public file
{
	DISALLOW_COPY_AND_ASSIGN(memlog_file);

	friend class file;

public:
	static cause::type setup();

	memlog_file() {}

	cause::type open();
	cause::type close();

	cause::type on_seek(s64 offset, int whence);
	cause::type on_read(iovec* iov, int iov_cnt, uptr* bytes);
	cause::type on_write(offset* off, int iov_cnt, const iovec* iov);

private:
	u8* buf;
	s64 current;
	s64 size;
};

extern memlog_file memlog;


#endif  // include guard

