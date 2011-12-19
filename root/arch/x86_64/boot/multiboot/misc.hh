/// @file   misc.hh
//
// (C) 2011 KATO Takeshi
//

#ifndef ARCH_X86_64_BOOT_MULTIBOOT_MISC_HH_
#define ARCH_X86_64_BOOT_MULTIBOOT_MISC_HH_

#include "cheap_alloc.hh"
#include "log_target.hh"


typedef cheap_alloc<256> allocator;
typedef cheap_alloc_separator<allocator> separator;

enum MEM_SLOT {
	SLOT_INDEX_NORMAL = 0,

	// カーネルへ jmp した直後にカーネルが RAM として使用可能なヒープを
	// BOOTHEAP と呼ぶことにする。
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

cause::stype pre_load(u32 magic, const u32* tag);
cause::stype post_load(u32* tag);

extern struct load_info_
{
	u64 entry_adr;
	u64 page_table_adr;

	u64 bootinfo_adr;
} load_info;


// log

class log : public log_target
{
public:
	log();
	~log();
};

void log_set(uint i, file* target);


class memlog_file : public file
{
public:
	static cause::stype setup();

	memlog_file() {}
	cause::stype open();
	cause::stype close();

private:
	static cause::stype op_seek(
	    file* x, s64 offset, int whence);
	cause::stype seek(s64 offset, int whence);

	static cause::stype op_read(
	    file* x, iovec* iov, int iov_cnt, uptr* bytes);
	cause::stype read(iovec* iov, int iov_cnt, uptr* bytes);

	static cause::stype op_write(
	    file* x, const iovec* iov, int iov_cnt, uptr* bytes);
	cause::stype write(const iovec* iov, int iov_cnt, uptr* bytes);

private:
	u8* buf;
	s64 current;
};


#endif  // include guard
