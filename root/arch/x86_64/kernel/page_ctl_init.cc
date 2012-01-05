/// @file  page_ctl_init.cc
/// @brief Page control initialize.
//
// (C) 2010-2011 KATO Takeshi
//

/// 物理アドレスに直接マッピングされた仮想アドレスのことを
/// Physical Address Map (PAM) と呼ぶことにする。
/// このソースコードの目的は物理アドレス全体の PAM を作ること。
///
/// カーネルに jmp した時点ではメモリの先頭 32MiB のヒープしか使えないと
/// 想定する。
/// そこで、最初に 32MiB のヒープを利用して 6GiB の PAM を作成する。
/// この 6GiB の PAM のことを PAM1 と呼ぶことにする。
///
/// 次に、PAM1 をヒープとして使い、メモリ全体をカバーする PAM を作成する。
///
/// メモリ全体をカバーする PAM を作成したら page_ctl を初期化し、
/// ページを管理できるようにする。
///
/// それ以降は page_ctl を通してメモリを管理する。

#include "arch.hh"
#include "basic_types.hh"
#include "bootinfo.hh"
#include "core_class.hh"
#include "cheap_alloc.hh"
#include "global_vars.hh"
#include "log.hh"
#include "page_ctl.hh"
#include "pagetable.hh"
#include "setupdata.hh"


namespace {

typedef cheap_alloc<64>                  tmp_alloc;
typedef cheap_alloc_separator<tmp_alloc> tmp_separator;

enum MEM_SLOT {
	SLOT_INDEX_HIGH     = 0,
	SLOT_INDEX_PAM1     = 1,
	SLOT_INDEX_BOOTHEAP = 2,
};
enum MEM_SLOT_MASK {
	SLOTM_HIGH     = 1 << SLOT_INDEX_HIGH,
	SLOTM_PAM1     = 1 << SLOT_INDEX_PAM1,
	SLOTM_BOOTHEAP = 1 << SLOT_INDEX_BOOTHEAP,

	SLOTM_ANY = SLOTM_HIGH | SLOTM_PAM1 | SLOTM_BOOTHEAP,
};
enum {
	BOOTHEAP_END = bootinfo::BOOTHEAP_END,
	SETUP_PAM1_MAPEND = U64(0x17fffffff), /* 6GiB */
};

const struct {
	tmp_alloc::slot_index slot;
	uptr slot_head;
	uptr slot_tail;
} memory_slots[] = {
	{ SLOT_INDEX_BOOTHEAP, 0,                     BOOTHEAP_END            },
	{ SLOT_INDEX_PAM1,     BOOTHEAP_END + 1,      SETUP_PAM1_MAPEND       },
	{ SLOT_INDEX_HIGH,     SETUP_PAM1_MAPEND + 1, U64(0xffffffffffffffff) },
};

void init_sep(tmp_separator* sep)
{
	for (u32 i = 0; i < sizeof memory_slots / sizeof memory_slots[0]; ++i)
	{
		sep->set_slot_range(
		    memory_slots[i].slot,
		    memory_slots[i].slot_head,
		    memory_slots[i].slot_tail);
	}
}

inline page_ctl* get_page_ctl() {
	return global_vars::gv.page_ctl_obj;
}

/// @brief 物理アドレス空間の末端アドレスを返す。
//
/// AVAILABLE でないアドレスも計算に含める。
u64 search_padr_end()
{
	const void* src = get_bootinfo(MULTIBOOT_TAG_TYPE_MMAP);
	if (src == 0)
		return 0;

	const multiboot_tag_mmap* mmap = (const multiboot_tag_mmap*)src;

	const void* mmap_end = (const u8*)mmap + mmap->size;
	const multiboot_mmap_entry* entry = mmap->entries;

	u64 adr_end = 0;
	while (entry < mmap_end) {
		adr_end = max<u64>(adr_end, entry->addr + entry->len - 1);

		entry = (const multiboot_mmap_entry*)
		    ((const u8*)entry + mmap->entry_size);
	}

	return adr_end;
}

/// @brief 物理メモリの末端アドレスを返す。
u64 search_pmem_end()
{
	const void* src = get_bootinfo(MULTIBOOT_TAG_TYPE_MMAP);
	if (src == 0)
		return 0;

	const multiboot_tag_mmap* mmap = (const multiboot_tag_mmap*)src;

	const void* mmap_end = (const u8*)mmap + mmap->size;
	const multiboot_mmap_entry* entry = mmap->entries;

	u64 pmem_end = 0;
	while (entry < mmap_end) {
		if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
			pmem_end = max<u64>(
			    pmem_end,
			    entry->addr + entry->len - 1);

		entry = (const multiboot_mmap_entry*)
		    ((const u8*)entry + mmap->entry_size);
	}

	return pmem_end;
}

/// @brief  物理的に存在するメモリを調べて tmp_separator に積む。
/// @retval true  Succeeds.
/// @retval false Physical memory info not found.
bool load_avail(tmp_separator* heap)
{
	const void* src = get_bootinfo(MULTIBOOT_TAG_TYPE_MMAP);
	if (src == 0)
		return false;

	const multiboot_tag_mmap* mmap = (const multiboot_tag_mmap*)src;

	const void* mmap_end = (const u8*)mmap + mmap->size;
	const multiboot_mmap_entry* entry = mmap->entries;

	while (entry < mmap_end) {
		if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
			heap->add_free(entry->addr, entry->len);
		}

		entry = (const multiboot_mmap_entry*)
		    ((const u8*)entry + mmap->entry_size);
	}

	return true;
}

/// @brief  前フェーズから使用中のメモリを tmp_alloc から外す。
/// @retval true  Succeeds.
/// @retval false Memory alloced info not found.
bool load_alloced(tmp_alloc* heap)
{
	const void* src = get_bootinfo(bootinfo::TYPE_MEMALLOC);
	if (src == 0)
		return false;

	const bootinfo::mem_alloc* ma = (const bootinfo::mem_alloc*)src;
	const void* end = (const u8*)ma + ma->size;
	const bootinfo::mem_alloc_entry* entry = ma->entries;
	while (entry < end) {
		heap->reserve(SLOTM_ANY, entry->adr, entry->bytes, true);
		++entry;
	}

	return true;
}

class page_table_tmpalloc
{
	tmp_alloc* tmpalloc;

public:
	page_table_tmpalloc(tmp_alloc* _alloc)
	    : tmpalloc(_alloc)
	{}
	cause::stype alloc(u64* padr);
	cause::stype free(u64 padr);
};

cause::stype page_table_tmpalloc::alloc(u64* padr)
{
	void* p = tmpalloc->alloc(
	    SLOTM_BOOTHEAP,
	    arch::page::PHYS_L1_SIZE,
	    arch::page::PHYS_L1_SIZE,
	    false);

	*padr = reinterpret_cast<u64>(p);

	return p != 0 ? cause::OK : cause::NO_MEMORY;
}

cause::stype page_table_tmpalloc::free(u64 padr)
{
	void* p = reinterpret_cast<void*>(padr);

	return tmpalloc->free(SLOTM_ANY, p) ? cause::OK : cause::FAIL;
}

inline u64 phys_to_virt_1(u64 padr) { return padr; }
inline u64 phys_to_virt_2(u64 padr) { return padr + arch::PHYSICAL_ADRMAP; }

typedef arch::page_table<page_table_tmpalloc, phys_to_virt_1> page_table_1;

/// @brief  Setup Physical Address Map 1st phase.
/// @param[in] padr_end  Maximum address.
/// @param[in] heap      Memory allocator.
/// @retval true  Succeeds.
/// @retval false Failed.
//
/// padr_end が 6GiB 以上の場合でも、最大 6GiB まで作成する。
cause::stype setup_pam1(u64 padr_end, tmp_alloc* heap)
{
	page_table_tmpalloc pt_alloc(heap);

	arch::pte* cr3 = reinterpret_cast<arch::pte*>(native::get_cr3());

	page_table_1 pg_tbl(cr3, &pt_alloc);

	padr_end = min<u64>(padr_end, SETUP_PAM1_MAPEND);

	for (uptr padr = 0; padr < padr_end; padr += arch::page::PHYS_L2_SIZE)
	{
		const cause::stype r = pg_tbl.set_page(
		    arch::PHYSICAL_ADRMAP + padr,
		    padr,
		    arch::page::PHYS_L2,
		    page_table_1::EXIST | page_table_1::WRITE);

		if (is_fail(r))
			return r;
	}

	return cause::OK;
}

cause::stype load_page(const tmp_alloc& alloc, page_ctl* ctl)
{
	tmp_alloc::enum_desc ea_enum;
	alloc.enum_free(SLOTM_ANY, &ea_enum);

	for (;;) {
		uptr adr, bytes;
		const bool r = alloc.enum_free_next(&ea_enum, &adr, &bytes);
		if (!r)
			break;
		ctl->load_free_range(adr, bytes);
	}

	return cause::OK;
}

bool setup_core_page(void* page)
{
	core_page* core_page_obj = new (page) core_page;

	log()("core_page size: ").u(sizeof *core_page_obj)();

	return core_page_obj->init();
}

}  // namespace


/// @retval cause::NO_MEMORY  No enough physical memory.
/// @retval cause::OK  Succeeds.
cause::stype page_ctl_init()
{
	tmp_alloc heap;
	tmp_separator sep(&heap);

	init_sep(&sep);

	if (load_avail(&sep) == false) {
		log()("Available memory detection failed.")();
		return cause::FAIL;
	}
	if (load_alloced(&heap) == false) {
		log()("Could not detect alloced memory. Might be abnormal.")();
	}

	const u64 padr_end = search_padr_end();

	cause::stype r = setup_pam1(padr_end, &heap);
	if (is_fail(r))
		return r;

	void* buf = heap.alloc(
	    SLOTM_BOOTHEAP,
	    arch::page::PHYS_L1_SIZE,
	    arch::page::PHYS_L1_SIZE,
	    false);
	if (!buf) {
		log()("bootheap is exhausted.")();
		return cause::NO_MEMORY;
	}
	buf = arch::map_phys_adr(buf, arch::page::PHYS_L1_SIZE);
	if (setup_core_page(buf) == false)
		return cause::UNKNOWN;

	page_ctl* pgctl = get_page_ctl();

	// 物理メモリの終端アドレス。これを物理メモリサイズとする。
	const uptr pmem_end = search_pmem_end();

	buf = heap.alloc(
	    SLOTM_BOOTHEAP,
	    pgctl->calc_workarea_size(pmem_end),
	    arch::page::PHYS_L1_SIZE,
	    false);
	if (!buf) {
		log()("bootheap is exhausted.")();
		return cause::NO_MEMORY;
	}

	pgctl->init(pmem_end, buf);

	r = load_page(heap, pgctl);
	if (r != cause::OK)
		return r;

	pgctl->build();

	return cause::OK;
}

