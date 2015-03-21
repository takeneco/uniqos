/// @file  page_ctl_init.cc
/// @brief Page control initialize.

//  UNIQOS  --  Unique Operating System
//  (C) 2010-2015 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

/// 物理アドレスに直接マッピングされた仮想アドレスのことを
/// Physical Address Map (PAM) と呼ぶことにする。
/// このソースコードの目的は物理アドレス全体の PAM を作ること。
///
/// カーネルに jmp した時点ではメモリの先頭 32MiB のヒープ(BOOT_HEAP)しか
/// 使えないと想定する。そこで、最初に 32MiB のヒープを利用して 6GiB の
/// PAM を作成する。この 6GiB の PAM のことを PAM1 と呼ぶことにする。
///
/// 次に、PAM1 をヒープとして使い、メモリ全体をカバーする PAM を作成する。
/// このソースコードではメモリ全体をカバーする PAM を PAM1 と区別するために
/// PAM2 と呼ぶ。
///
/// PAM2 を作成したら page_ctl を初期化し、ページを管理できるようにする。
/// それ以降は page_ctl を通してメモリを管理する。

//TODO:ここのログは機能しない

#include <page_ctl.hh>

#include <arch/native_ops.hh>
#include <bootinfo.hh>
#include <cheap_alloc.hh>
#include <config.h>
#include <core/log.hh>
#include <core/page.hh>
#include <core/page_pool.hh>
#include <global_vars.hh>
#include <mpspec.hh>
#include <native_cpu_node.hh>
#include "page_table.hh"

#if CONFIG_ACPI
# include <core/acpi_ctl.hh>
#endif  // CONFIG_ACPI

#define SEGINIT __attribute__((section(".text.init")))


namespace {

const uptr KERNEL_ADR_SPACE_START = U64(0x0000800000000000);
const uptr KERNEL_ADR_SPACE_END   = U64(0x0000ffffffffffff);

typedef cheap_alloc<64>                  tmp_alloc;
typedef cheap_alloc_separator<tmp_alloc> tmp_separator;

enum MEM_SLOT {
	SLOT_INDEX_PAM2     = 0,
	SLOT_INDEX_PAM1     = 1,
	SLOT_INDEX_BOOTHEAP = 2,
};
enum MEM_SLOT_MASK {
	SLOTM_PAM2     = 1 << SLOT_INDEX_PAM2,
	SLOTM_PAM1     = 1 << SLOT_INDEX_PAM1,
	SLOTM_BOOTHEAP = 1 << SLOT_INDEX_BOOTHEAP,

	SLOTM_ANY = SLOTM_PAM2 | SLOTM_PAM1 | SLOTM_BOOTHEAP,
};
enum {
	BOOTHEAP_END = bootinfo::BOOTHEAP_END,
	SETUP_PAM1_MAPEND = U64(0x17fffffff), /* 6GiB */
};

const struct {
	tmp_alloc::slot_index slot;
	uptr slot_head;
	uptr slot_tail;
} setup_heap_slots[] = {
	{ SLOT_INDEX_BOOTHEAP, 0,                     BOOTHEAP_END            },
	{ SLOT_INDEX_PAM1,     BOOTHEAP_END + 1,      SETUP_PAM1_MAPEND       },
	{ SLOT_INDEX_PAM2,     SETUP_PAM1_MAPEND + 1, U64(0xffffffffffffffff) },
};


/// @brief 物理アドレス空間の末端アドレスを返す。
//
/// AVAILABLE でないアドレスも計算に含める。
u64 search_padr_end()
{
	const void* info = bootinfo::get_info(bootinfo::TYPE_ADR_MAP);
	if (info == 0)
		return 0;

	const bootinfo::adr_map* adrmap =
	    static_cast<const bootinfo::adr_map*>(info);

	const void* end = adrmap->end_entry();
	const bootinfo::adr_map::entry* entry = adrmap->entries;

	u64 adr_end = 0;
	while (entry < end) {
		adr_end = max<u64>(adr_end, entry->adr + entry->bytes - 1);

		++entry;
	}

	return adr_end;
}

/// @brief  物理的に存在するメモリを調べて tmp_separator に積む。
/// @retval cause::OK   成功した。
/// @retval cause::FAIL メモリの情報が見つからないか、tmp_alloc があふれた。
cause::t load_avail(tmp_separator* heap)
{
	const void* info = bootinfo::get_info(bootinfo::TYPE_ADR_MAP);
	if (info == 0)
		return cause::FAIL;

	const bootinfo::adr_map* adrmap =
	    static_cast<const bootinfo::adr_map*>(info);

	const void* end = adrmap->end_entry();
	const bootinfo::adr_map::entry* entry = adrmap->entries;

	while (entry < end) {
		if (entry->type == bootinfo::adr_map::entry::AVAILABLE) {
			if (!heap->add_free(entry->adr, entry->bytes))
				return cause::FAIL;
		}

		++entry;
	}

	return cause::OK;
}

/// @brief  前フェーズから使用中のメモリを tmp_alloc から外す。
/// @retval cause::OK   成功した。
/// @retval cause::FAIL メモリの情報が見つからないか、tmp_alloc があふれた。
cause::t load_allocated(tmp_alloc* heap)
{
	const void* info = bootinfo::get_info(bootinfo::TYPE_MEM_ALLOC);
	if (info == 0)
		return cause::FAIL;

	const bootinfo::mem_alloc* ma =
	    static_cast<const bootinfo::mem_alloc*>(info);

	const void* end = ma->end_entry();

	const bootinfo::mem_alloc::entry* entry = ma->entries;

	while (entry < end) {
		if (!heap->reserve(SLOTM_ANY, entry->adr, entry->bytes, true))
			return cause::FAIL;
		++entry;
	}

	return cause::OK;
}

/// @brief ヒープを作る。
cause::t setup_heap(tmp_alloc* heap)
{
	tmp_separator sep(heap);

	for (u32 i = 0; i < num_of_array(setup_heap_slots); ++i) {
		sep.set_slot_range(
		    setup_heap_slots[i].slot,
		    setup_heap_slots[i].slot_head,
		    setup_heap_slots[i].slot_tail);
	}

	cause::t r = load_avail(&sep);
	if (is_fail(r)) {
		// unavailable
		//log()("!!! Available memory detection failed.")();
		return r;
	}

	r = load_allocated(heap);
	if (is_fail(r)) {
		// unavailable
		//log()("!!! Could not detect alloced memory.")();
		return r;
	}

	return cause::OK;
}

// Page table for PAM1.

class pam1_page_table_traits;
typedef arch::page_table_tmpl<pam1_page_table_traits> _pam1_page_table;
class pam1_page_table : public _pam1_page_table
{
public:
	pam1_page_table(arch::pte* top, tmp_alloc* _heap) :
		_pam1_page_table(top), heap(_heap)
	{}

	tmp_alloc* heap;
};
class pam1_page_table_traits
{
public:
	static cause::pair<uptr> acquire_page(_pam1_page_table* x)
	{
		pam1_page_table* _x = static_cast<pam1_page_table*>(x);

		void* p = _x->heap->alloc(
		    SLOTM_BOOTHEAP,
		    arch::page::PHYS_L1_SIZE,
		    arch::page::PHYS_L1_SIZE,
		    true);

		return cause::pair<uptr>(
		    p != 0 ? cause::OK : cause::NOMEM,
		    reinterpret_cast<uptr>(p));
	}

	static void* phys_to_virt(uptr padr)
	{
		return reinterpret_cast<void*>(padr);
	}
};

// page table for PAM2.

class pam2_page_table_traits;
typedef arch::page_table_tmpl<pam2_page_table_traits> _pam2_page_table;
class pam2_page_table : public _pam2_page_table
{
public:
	pam2_page_table(arch::pte* top, tmp_alloc* _heap) :
		_pam2_page_table(top), heap(_heap)
	{}

	tmp_alloc* heap;
};
class pam2_page_table_traits
{
public:
	static cause::pair<uptr> acquire_page(_pam2_page_table* x)
	{
		pam2_page_table* _x = static_cast<pam2_page_table*>(x);

		void* p = _x->heap->alloc(
		    SLOTM_BOOTHEAP | SLOTM_PAM1,
		    arch::page::PHYS_L1_SIZE,
		    arch::page::PHYS_L1_SIZE,
		    true);

		return cause::pair<uptr>(
		    p != 0 ? cause::OK : cause::NOMEM,
		    reinterpret_cast<uptr>(p));
	}

	static void* phys_to_virt(u64 padr)
	{
		return reinterpret_cast<void*>(arch::PHYS_MAP_ADR + padr);
	}
};

/// @brief  Setup physical mapping address 1st phase.
/// @param[in] padr_end  Maximum address.
/// @param[in] heap      Memory allocator.
/// @retval true  Succeeds.
/// @retval false Failed.
//
/// BOOTHEAP だけを使い、padr_end が 6GiB 以上の場合でも
/// 最大 6GiB まで作成する。
cause::t setup_pam1(u64 padr_end, tmp_alloc* heap)
{
	arch::pte* cr3 = reinterpret_cast<arch::pte*>(native::get_cr3());

	pam1_page_table pg_tbl(cr3, heap);

	padr_end = min<u64>(padr_end, SETUP_PAM1_MAPEND);

	for (uptr padr = 0; padr < padr_end; padr += arch::page::PHYS_L2_SIZE)
	{
		const cause::t r = pg_tbl.set_page(
		    arch::PHYS_MAP_ADR + padr,
		    padr,
		    arch::page::PHYS_L2,
		    pam1_page_table::EXIST | pam1_page_table::WRITE);

		if (is_fail(r))
			return r;
	}

	return cause::OK;
}

/// @brief  Setup physical mapping address 2nd phase.
/// @param[in] padr_end  Maximum address.
/// @param[in] heap      Memory allocator.
/// @retval true  Succeeds.
/// @retval false Failed.
cause::t setup_pam2(u64 padr_end, tmp_alloc* heap)
{
	arch::pte* cr3 = static_cast<arch::pte*>(
	    arch::map_phys_adr(native::get_cr3(), arch::page::PHYS_L1_SIZE));

	pam2_page_table pg_tbl(cr3, heap);

	for (uptr padr = SETUP_PAM1_MAPEND + 1;
	     padr < padr_end;
	     padr += arch::page::PHYS_L2_SIZE)
	{
		const cause::t r = pg_tbl.set_page(
		    arch::PHYS_MAP_ADR + padr,
		    padr,
		    arch::page::PHYS_L2,
		    pam2_page_table::EXIST | pam2_page_table::WRITE);

		if (is_fail(r))
			return r;
	}

	return cause::OK;
}

/// @brief ヒープから 1 ページ分のメモリを確保する。
//
/// ページのサイズは計算して決める。
/// 確保したメモリはページ管理ができるようになってから
/// page_heap_dealloc() で開放する。
void* page_heap_alloc(tmp_alloc* heap, uptr bytes)
{
	bytes += sizeof (cpu_word);

	const arch::page::TYPE page_type = arch::page::type_of_size(bytes);
	const uptr page_sz = size_of_type(page_type);

	cpu_word* p = static_cast<cpu_word*>(
	    heap->alloc(SLOTM_ANY, page_sz, page_sz, false));

	if (!p)
		log()("!!! No enough memory.")();

	p[0] = page_type;

	return arch::map_phys_adr(&p[1], page_sz);
}

/// @brief page_heap_alloc() で確保したメモリを開放する。
cause::t page_heap_dealloc(void* p)
{
	cpu_word* page = static_cast<cpu_word*>(p);
	page -= 1;

	const arch::page::TYPE page_type =
	    static_cast<arch::page::TYPE>(page[0]);

	const uptr padr = arch::unmap_phys_adr(page, size_of_type(page_type));

	return page_dealloc(page_type, padr);
}

/// @brief ヒープから tmp_alloc 用のメモリを確保する。
//
/// 後でページ単位で開放できるように、メモリをページ単位で確保する。
void* allocate_tmp_alloc(tmp_alloc* heap)
{
	const uptr tmp_alloc_pagesz =
	    size_of_type(arch::page::type_of_size(sizeof (tmp_alloc)));

	void* p =
	    heap->alloc(SLOTM_ANY, tmp_alloc_pagesz, tmp_alloc_pagesz, false);

	if (!p)
		log()("!!! No enough memory.")();

	return arch::map_phys_adr(p, tmp_alloc_pagesz);
}

// freeメモリだけを src から dest へ移動する。
// allocメモリは移動しない。
bool move_free_range(
    uptr start_adr, uptr end_adr, tmp_alloc* src, tmp_alloc* dest)
{
	tmp_alloc::enum_desc ed;
	src->enum_free(SLOTM_ANY, &ed);

	for (;;) {
		adr_range tmp_ar;
		if (!src->enum_free_next(&ed, &tmp_ar))
			break;

		uptr low_adr = tmp_ar.low_adr();
		uptr high_adr = tmp_ar.high_adr();
		if (low_adr < start_adr)
			low_adr = start_adr;
		if (high_adr > end_adr)
			high_adr = end_adr;

		if (low_adr > high_adr)
			continue;

		if (!dest->add_free(0, adr_range::gen_lh(low_adr, high_adr)))
			return false;
	}

	if (!src->reserve(SLOTM_ANY, adr_range::gen_lh(start_adr, end_adr), true))
		return false;

	return true;
}

u32 acpi_count_memory_affinity(ACPI_TABLE_SRAT* srat)
{
	acpi::subtable_enumerator subtbl_enum(srat);
	u32 cnt = 0;

	for (auto* e = subtbl_enum.next(ACPI_SRAT_TYPE_MEMORY_AFFINITY);
	     e;
	     e = subtbl_enum.next(ACPI_SRAT_TYPE_MEMORY_AFFINITY))
	{
		++cnt;
	}

	return cnt;
}

cause::pair<page_pool*> create_page_pool(
    uptr base_adr,
    uptr length,
    u32 proximity_domain,
    tmp_alloc* aff_heap)
{
	// calc page_pool working buffer size.
	page_pool tmp_pp(proximity_domain);
	tmp_pp.add_range(adr_range::gen_ab(base_adr, length));
	uptr buf_bytes = tmp_pp.calc_workbuf_bytes();

	//TODO:SLOTM_ANY
	void* pp_mem = aff_heap->alloc(SLOTM_ANY,
	    sizeof (page_pool) + buf_bytes,
	    arch::BASIC_TYPE_ALIGN,
	    false);
	if (!pp_mem)
		return null_pair(cause::NOMEM);
	void* pp_vadr =
	    arch::map_phys_adr(pp_mem, sizeof (page_pool) + buf_bytes);
	page_pool* pp = new (pp_vadr) page_pool(proximity_domain);

	pp->copy_range_from(tmp_pp);

	if (!pp->init(buf_bytes, pp + 1))
		return null_pair(cause::UNKNOWN);

	tmp_alloc::enum_desc ed;
	aff_heap->enum_free(SLOTM_ANY, &ed);
	for (;;) {
		uptr adr, bytes;
		if (!aff_heap->enum_free_next(&ed, &adr, &bytes))
			break;

		pp->load_free_range(adr, bytes);
	}

	pp->build();

	return make_pair(cause::OK, pp);
}

/// allocate global_vars::core.page_pool_objs
/// and set global_vars::core.page_pool_nr
cause::t allocate_page_pool_array(tmp_alloc* heap, u32 nr)
{
	void* pp_array_mem = heap->alloc(
	    SLOTM_ANY,
	    sizeof (page_pool*[nr]),
	    arch::BASIC_TYPE_ALIGN,
	    false);
	if (!pp_array_mem)
		return cause::NOMEM;

	void* pp_array_vadr =
	    arch::map_phys_adr(pp_array_mem, sizeof (page_pool*[nr]));
	page_pool** pp_array = new (pp_array_vadr) page_pool*[nr];

	global_vars::core.page_pool_objs = pp_array;
	global_vars::core.page_pool_nr = nr;

	return cause::OK;
}

cause::t setup_page_pool_by_srat(tmp_alloc* heap)
{
#if CONFIG_ACPI

	// initialize ACPI table.

	void* acpi_buffer = heap->alloc(SLOTM_ANY,
	                                arch::page::L1_SIZE,  // size
	                                arch::page::L1_SIZE,  // align
	                                true);                // forget
	if (!acpi_buffer)
		return cause::NOMEM;

	cause::t r = acpi::table_init(
	    reinterpret_cast<uptr>(acpi_buffer),
	    arch::page::L1);
	if (is_fail(r))
		return r;

	// get SRAT table.
	ACPI_TABLE_SRAT* srat;
	char sig_srat[] = ACPI_SIG_SRAT;
	ACPI_STATUS as = AcpiGetTable(
	    sig_srat, 0, reinterpret_cast<ACPI_TABLE_HEADER**>(&srat));
	if (ACPI_FAILURE(as))
		return cause::FAIL;

	// create page_pool array.

	u32 memaff_nr = acpi_count_memory_affinity(srat);
	if (memaff_nr == 0)
		return cause::FAIL;

	r = allocate_page_pool_array(heap, memaff_nr);
	if (is_fail(r))
		return r;

	void* aff_heap_mem = allocate_tmp_alloc(heap);

	acpi::subtable_enumerator subtbl_enum(srat);
	for (u32 i = 0; i < memaff_nr; ++i) {
		// create page_pool instances.

		auto* mem_aff = reinterpret_cast<ACPI_SRAT_MEM_AFFINITY*>(
		    subtbl_enum.next(ACPI_SRAT_TYPE_MEMORY_AFFINITY));

		tmp_alloc* aff_heap = new (aff_heap_mem) tmp_alloc;

		if (!move_free_range(
		    mem_aff->BaseAddress,
		    mem_aff->BaseAddress + mem_aff->Length - 1,
		    heap,
		    aff_heap))
		{
			return cause::FAIL;
		}

		auto pp = create_page_pool(
		    mem_aff->BaseAddress,
		    mem_aff->Length,
		    mem_aff->ProximityDomain,
		    aff_heap);
		if (is_fail(pp))
			return pp.cause();

		global_vars::core.page_pool_objs[i] = pp.data();

		aff_heap->~tmp_alloc();
		operator delete(aff_heap, aff_heap_mem);
	}

	//TODO: free aff_heap_mem

#endif  // CONFIG_ACPI
	return cause::FAIL;
}

cause::t setup_page_pool_by_bootinfo(tmp_alloc* heap)
{
	cause::t r = allocate_page_pool_array(heap, 1);
	if (is_fail(r))
		return r;

	uptr padr_end = search_padr_end();

	auto pp = create_page_pool(0, padr_end, 0, heap);
	if (is_fail(pp))
		return pp.cause();

	global_vars::core.page_pool_objs[0] = pp.data();

	return cause::OK;
}

cause::t setup_page_pool(tmp_alloc* heap)
{
	cause::t r = setup_page_pool_by_srat(heap);
	if (is_ok(r))
		return r;

	r = setup_page_pool_by_bootinfo(heap);
	if (is_ok(r))
		return r;

	return cause::FAIL;
}

/// @brief  page_pool から1ページを割り当てて native_cpu_node を作る。
/// @retval r=cause::OK Succeeds, value is pointer to native_cpu_node.
cause::pair<x86::native_cpu_node*> create_native_cpu_node(
    page_pool* pp, cpu_id cpunode_id)
{
	typedef cause::pair<x86::native_cpu_node*> ret_type;

	// native_cpu_node はCPUごとに別ページになるように１ページを
	// 固定で割り当てる。
	// native_cpu_node のサイズが１ページを超えたらソースレベルで
	// 何とかする。
	if (sizeof (x86::native_cpu_node) <= arch::page::PHYS_L1_SIZE) {
		/*
		if (CONFIG_DEBUG_VERBOSE >= 1) {
			log()("sizeof (native_cpu_node) : ")
			    .u(sizeof (x86::native_cpu_node))
			    (" <= ")
			    ("PHYS_L1_SIZE : ")
			    .u(arch::page::PHYS_L1_SIZE)();
		}
		*/
	} else {
		log()(SRCPOS)("!!! sizeof (native_cpu_node) is too large.")();
		return ret_type(cause::UNKNOWN, 0);
	}

	uptr page_adr;
	cause::t r = pp->alloc(arch::page::PHYS_L1, &page_adr);
	if (is_fail(r)) {
		log()("!!! No enough memory.")();
		return ret_type(cause::NOMEM, 0);
	}

	void* buf = arch::map_phys_adr(page_adr, arch::page::PHYS_L1_SIZE);

	x86::native_cpu_node* ncn = new (buf) x86::native_cpu_node(cpunode_id);

	return ret_type(cause::OK, ncn);
}

void set_page_pool_to_cpu_node(cpu_id cpu_node_id)
{
	page_pool** const pps = global_vars::core.page_pool_objs;

	cpu_node* cn = global_vars::core.cpu_node_objs[cpu_node_id];

	const int n = global_vars::core.page_pool_nr;
	cn->set_page_pool_cnt(n);

	for (int i = 0; i < n; ++i) {
		cn->set_page_pool(i, pps[i]);
	}
}

cause::t setup_cpu_node_by_mpspec()
{
	mpspec mps;
	cause::t r = mps.load();
	if (is_fail(r))
		return r;

	page_pool* pp = global_vars::core.page_pool_objs[0];

	mpspec::processor_iterator proc_itr(&mps);
	cpu_id proc_nr = 0;
	for (;;) {
		const mpspec::processor_entry* pe = proc_itr.get_next();
		if (!pe)
			break;

		auto rcn = create_native_cpu_node(pp, pe->localapic_id);
		if (is_fail(rcn))
			return rcn.cause();

		global_vars::core.cpu_node_objs[proc_nr] = rcn.data();

		++proc_nr;
	}

	global_vars::core.cpu_node_nr = proc_nr;

	return cause::OK;
}

cause::t setup_cpu_node_one()
{
	global_vars::core.cpu_node_nr = 1;

	page_pool* pp = global_vars::core.page_pool_objs[0];
	auto rcn = create_native_cpu_node(pp, arch::get_cpu_lapic_id());
	if (is_fail(rcn))
		return rcn.cause();

	global_vars::core.cpu_node_objs[0] = rcn.data();

	return cause::OK;
}

cause::t setup_cpu_node()
{
	cause::t r = setup_cpu_node_by_mpspec();
	if (is_ok(r))
		return r;

	r = setup_cpu_node_one();
	if (is_ok(r))
		return r;

	return cause::FAIL;
}

cause::t setup_cpu_page()
{
	cpu_id n = global_vars::core.cpu_node_nr;
	for (cpu_id i = 0; i < n; ++i) {
		set_page_pool_to_cpu_node(i);
	}

	return cause::OK;
}

cause::t init_kern_space()
{
	arch::pte* cr3 = static_cast<arch::pte*>(
	    arch::map_phys_adr(native::get_cr3(), arch::page::PHYS_L1_SIZE));

	x86::page_table pg_tbl(cr3);

	for (uptr padr = KERNEL_ADR_SPACE_START;
	     padr < KERNEL_ADR_SPACE_END;
	     padr += arch::page::PHYS_L4_SIZE)
	{
		auto r = pg_tbl.declare_table(padr, arch::page::PHYS_L4);

		if (is_fail(r))
			return r.r;
	}

	return cause::OK;
}

}  // namespace


cause::t cpu_page_init()
{
	tmp_alloc heap;
	setup_heap(&heap);

	const u64 padr_end = search_padr_end();

	cause::t r = setup_pam1(padr_end, &heap);
	if (is_fail(r))
		return r;

	r = setup_pam2(padr_end, &heap);
	if (is_fail(r))
		return r;

	r = setup_page_pool(&heap);
	if (is_fail(r))
		return r;

	for (int i = 0; i < CONFIG_MAX_CPUS; ++i)
		global_vars::core.cpu_node_objs[i] = 0;

	r = setup_cpu_node();
	if (is_fail(r))
		return r;

	r = setup_cpu_page();
	if (is_fail(r))
		return r;

	r = init_kern_space();
	if (is_fail(r))
		return r;

	return cause::OK;
}

