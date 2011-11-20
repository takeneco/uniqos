/// @file  physical_memory.cc
/// @brief Physical memory management.
//
// (C) 2010-2011 KATO Takeshi
//

#include "arch.hh"
#include "basic_types.hh"
#include "bootinfo.hh"
#include "core_class.hh"
#include "easy_alloc.hh"
#include "global_vars.hh"
#include "page_ctl.hh"
#include "pagetable.hh"
#include "placement_new.hh"
#include "setupdata.hh"
#include "boot_access.hh"


void page_ctl::detect_paging_features()
{
	struct regs {
		u32 eax, ebx, ecx, edx;
	} r[3];

	asm volatile ("cpuid" :
	    "=a"(r[0].eax), "=b"(r[0].ebx), "=c"(r[0].ecx), "=d"(r[0].edx) :
	    "a"(0x01));
	asm volatile ("cpuid" :
	    "=a"(r[1].eax), "=b"(r[1].ebx), "=c"(r[1].ecx), "=d"(r[1].edx) :
	    "a"(0x80000001));
	asm volatile ("cpuid" :
	    "=a"(r[2].eax), "=b"(r[2].ebx), "=c"(r[2].ecx), "=d"(r[2].edx) :
	    "a"(0x80000008));

	pse     = !!(r[0].edx & 0x00000008);
	pae     = !!(r[0].edx & 0x00000040);
	pge     = !!(r[0].edx & 0x00002000);
	pat     = !!(r[0].edx & 0x00010000);
	pse36   = !!(r[0].edx & 0x00020000);
	pcid    = !!(r[0].ecx & 0x00020000);

	nx      = !!(r[1].edx & 0x00100000);
	page1gb = !!(r[1].edx & 0x02000000);
	lm      = !!(r[1].edx & 0x20000000);

	padr_width = r[2].eax & 0x000000ff;
	vadr_width = (r[2].eax & 0x0000ff00) >> 8;
/*
	log()
	("pse=").u(u8(pse))
	(";pae=").u(u8(pae))
	(";pge=").u(u8(pge))
	(";pat=").u(u8(pat))
	(";pse36=").u(u8(pse36))
	(";pcid=").u(u8(pcid))
	(";nx=").u(u8(nx))
	(";page1gb=").u(u8(page1gb))
	(";lm=").u(u8(lm))
	(";padr_width=").u(u8(padr_width))
	(";vadr_width=").u(u8(vadr_width))();
*/
}

page_ctl::page_ctl()
{
	page_base[0].set_params(12, 0);
	page_base[1].set_params(18, &page_base[0]);
	page_base[2].set_params(21, &page_base[1]);
	page_base[3].set_params(27, &page_base[2]);
	page_base[4].set_params(30, &page_base[3]);
}

/// @brief 物理メモリの管理に必要なデータエリアのサイズを返す。
//
/// @param[in] _pmem_end 物理アドレスの終端アドレス。
/// @return ワークエリアのサイズをバイト数で返す。
uptr page_ctl::calc_workarea_size(uptr _pmem_end)
{
	return page_base[4].calc_buf_size(_pmem_end);
}

/// @param[in] _pmem_end 物理メモリの終端アドレス。
/// @param[in] buf  calc_workarea_size() が返したサイズのメモリへのポインタ。
/// @return true を返す。
bool page_ctl::init(uptr _pmem_end, void* buf)
{
	page_base[4].set_buf(buf, _pmem_end);

	detect_paging_features();

	return true;
}

bool page_ctl::load_setupdump()
{
	setup_memory_dumpdata* freemap;
	u32 freemap_num;
	setup_get_free_memdump(&freemap, &freemap_num);

	for (u32 i = 0; i < freemap_num; ++i) {
		page_base[4].free_range(
		    freemap[i].head,
		    freemap[i].head + freemap[i].bytes - 1);
	}

	return true;
}

bool page_ctl::load_free_range(u64 adr, u64 bytes)
{
	page_base[4].free_range(adr, adr + bytes - 1);

	return true;
}

void page_ctl::build()
{
	page_base[4].build_free_chain();
}

cause::stype page_ctl::alloc(arch::page::TYPE page_type, uptr* padr)
{
	return page_base[page_type].reserve_1page(padr);
}

cause::stype page_ctl::free(arch::page::TYPE page_type, uptr padr)
{
	return page_base[page_type].free_1page(padr);
}

namespace {

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


typedef easy_alloc<32> tmp_alloc;

bool load_mem_avail_classify(
    u64 adr, u64 len,
    tmp_alloc* bootheap,
    tmp_alloc* largeheap)
{
	// 先頭16MiBはなるべく空けておく。
	const u64 AVOID_END = 0xffffff;

	const u64 BOOTHEAP_END = bootinfo::BOOTHEAP_END;

	if (adr > BOOTHEAP_END) {
		largeheap->add_free(adr, len, false);
		return true;
	}
	else if (BOOTHEAP_END < adr + len) {
		largeheap->add_free(
		    BOOTHEAP_END + 1,
		    adr + len - BOOTHEAP_END + 1,
		    false);
		len = BOOTHEAP_END + 1 - adr;
	}

	if (adr > AVOID_END) {
		bootheap->add_free(adr, len, false);
		return true;
	}
	else if (AVOID_END < adr + len) {
		bootheap->add_free(
		    AVOID_END + 1,
		    adr + len - AVOID_END + 1,
		    false);
		len = AVOID_END + 1 - adr;
	}

	bootheap->add_free(adr, len, true);

	return true;
}

bool load_mem_avail(tmp_alloc* bootheap, tmp_alloc* largeheap)
{
	const void* src = get_bootinfo(MULTIBOOT_TAG_TYPE_MMAP);
	if (src == 0)
		return false;

	const multiboot_tag_mmap* mmap = (const multiboot_tag_mmap*)src;

	const void* mmap_end = (const u8*)mmap + mmap->size;
	const multiboot_mmap_entry* entry = mmap->entries;

	while (entry < mmap_end) {
		if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
			load_mem_avail_classify(
			    entry->addr, entry->len, bootheap, largeheap);
		}

		entry = (const multiboot_mmap_entry*)
		    ((const u8*)entry + mmap->entry_size);
	}

	return true;
}

bool load_mem_state(tmp_alloc* bootheap, tmp_alloc* largeheap)
{
	const void* src = get_bootinfo(bootinfo::TYPE_MEMALLOC);
	if (src == 0)
		return false;

	const bootinfo::mem_alloc* ma = (const bootinfo::mem_alloc*)src;
	const void* end = (const u8*)ma + ma->size;
	const bootinfo::mem_alloc_entry* entry = ma->entries;
	while (entry < end) {
		bootheap->reserve(entry->adr, entry->bytes, true);
		largeheap->reserve(entry->adr, entry->bytes, true);
		++entry;
	}

	return true;
}

class page_table_tmpalloc
{
	tmp_alloc* tmpalloc1;
	tmp_alloc* tmpalloc2;

public:
	page_table_tmpalloc(tmp_alloc* _alloc1, tmp_alloc* _alloc2=0)
	    : tmpalloc1(_alloc1), tmpalloc2(_alloc2)
	{}
	cause::stype alloc(u64* padr);
	cause::stype free(u64 padr);
};

cause::stype page_table_tmpalloc::alloc(u64* padr)
{
	void* p = tmpalloc1->alloc(
	    arch::page::PHYS_L1_SIZE,
	    arch::page::PHYS_L1_SIZE,
	    false);

	if (!p && tmpalloc2) {
		p = tmpalloc2->alloc(
		    arch::page::PHYS_L1_SIZE,
		    arch::page::PHYS_L1_SIZE,
		    false);
	}

	*padr = reinterpret_cast<u64>(p);

	return p != 0 ? cause::OK : cause::NO_MEMORY;
}

cause::stype page_table_tmpalloc::free(u64 padr)
{
	void* p = reinterpret_cast<void*>(padr);

	bool r = tmpalloc1->free(p);

	if (!r)
		r = tmpalloc2->free(p);

	return r ? cause::OK : cause::FAIL;
}

inline u64 phys_to_virt_1(u64 padr) { return padr; }
inline u64 phys_to_virt_2(u64 padr) { return padr + arch::PHYSICAL_ADRMAP; }

typedef arch::page_table<page_table_tmpalloc, phys_to_virt_1> page_table_1;

enum { SETUP_PAM1_MAPEND = U64(0x17fffffff) /* 6GiB */ };

/// @brief  Setup Physical Address Mapping 1st phase.
/// @param[in] padr_end  Maximum address.
/// @param[in] bootheap  Memory allocator.
/// @retval true  Succeeds.
/// @retval false Failed.
//
/// padr_end が 6GiB 以上の場合でも、最大 6GiB まで作成する。
cause::stype setup_pam1(u64 padr_end, tmp_alloc* bootheap)
{
	page_table_tmpalloc tmpalloc(bootheap);

	arch::pte* cr3 = reinterpret_cast<arch::pte*>(native::get_cr3());

	page_table_1 pg_tbl(cr3, &tmpalloc);

	padr_end = min<u64>(padr_end, SETUP_PAM1_MAPEND);

	for (uptr padr = 0; padr < padr_end; padr += arch::page::PHYS_L2_SIZE)
	{
		cause::stype r = pg_tbl.set_page(
		    arch::PHYSICAL_ADRMAP + padr,
		    padr,
		    arch::page::PHYS_L2,
		    page_table_1::EXIST | page_table_1::WRITE);

		if (r != cause::OK)
			return r;
	}

arch::dump_pte(log(), cr3, 1);
	return cause::OK;
}

cause::stype load_page(const tmp_alloc& alloc, page_ctl* ctl)
{
	easy_alloc_enum ea_enum;
	alloc.all_free_info(&ea_enum);

	for (;;) {
		uptr adr, bytes;
		const bool r = alloc.all_free_info_next(&ea_enum, &adr, &bytes);
		if (!r)
			break;
		ctl->load_free_range(adr, bytes);
	}

	return cause::OK;
}

bool setup_core_page(void* page)
{
	core_page* core_page_obj = new (page) core_page;

	return core_page_obj->init();
}

}  // namespace

namespace arch {
namespace page {

/// @retval cause::NO_MEMORY  No enough physical memory.
/// @retval cause::OK  Succeeds.
cause::stype init()
{
	tmp_alloc bootheap, largeheap;
	bootheap.init();
	largeheap.init();

	load_mem_avail(&bootheap, &largeheap);
	load_mem_state(&bootheap, &largeheap);

	const u64 padr_end = search_padr_end();

	setup_pam1(padr_end, &bootheap);

	void* buf = bootheap.alloc(PHYS_L1_SIZE, PHYS_L1_SIZE, false);
	buf = arch::map_phys_adr(buf, PHYS_L1_SIZE);
	if (setup_core_page(buf) == false)
		return cause::UNKNOWN;

	page_ctl* pgctl = get_page_ctl();

	// 物理メモリの終端アドレス。これを物理メモリサイズとする。
	const uptr pmem_end = search_pmem_end();

	buf = bootheap.alloc(
	    pgctl->calc_workarea_size(pmem_end), PHYS_L1_SIZE, false);
	if (buf == 0)
		return cause::NO_MEMORY;

	pgctl->init(pmem_end, buf);

	cause::stype r = load_page(bootheap, pgctl);
	if (r != cause::OK)
		return r;

	pgctl->build();

	return cause::OK;
}

cause::stype alloc(TYPE page_type, uptr* padr)
{
	return global_vars::gv.page_ctl_obj->alloc(page_type, padr);
}

cause::stype free(TYPE page_type, uptr padr)
{
	return global_vars::gv.page_ctl_obj->free(page_type, padr);
}

}  // namespace page
}  // namespace arch

