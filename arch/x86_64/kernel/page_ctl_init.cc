/// @file  page_ctl_init.cc
/// @brief Page control initialize.
//
// (C) 2010-2013 KATO Takeshi
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

//TODO:ここのログは機能しない

#include <page_ctl.hh>

#include <acpi_ctl.hh>
#include <arch.hh>
#include <bootinfo.hh>
#include <cheap_alloc.hh>
#include <config.h>
#include <global_vars.hh>
#include <log.hh>
#include <mpspec.hh>
#include <native_cpu_node.hh>
#include <native_ops.hh>
#include <page.hh>
#include <page_pool.hh>
#include <pagetable.hh>


namespace {

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
	const void* src = bootinfo::get_info(bootinfo::TYPE_ADR_MAP);
	if (src == 0)
		return 0;

	const bootinfo::adr_map* adrmap =
	    static_cast<const bootinfo::adr_map*>(src);

	const void* end = adrmap->next();
	const bootinfo::adr_map::entry* ent = adrmap->entries;

	u64 adr_end = 0;
	while (ent < end) {
		adr_end = max<u64>(adr_end, ent->adr + ent->len - 1);

		++ent;
	}

	return adr_end;
}

/// @brief  物理的に存在するメモリを調べて tmp_separator に積む。
/// @retval cause::OK   成功した。
/// @retval cause::FAIL メモリの情報が見つからないか、tmp_alloc があふれた。
cause::t load_avail(tmp_separator* heap)
{
	const void* src = bootinfo::get_info(bootinfo::TYPE_ADR_MAP);
	if (src == 0)
		return cause::FAIL;

	const bootinfo::adr_map* adrmap =
	    static_cast<const bootinfo::adr_map*>(src);

	const void* end = adrmap->next();
	const bootinfo::adr_map::entry* ent = adrmap->entries;

	while (ent < end) {
		if (ent->type == bootinfo::adr_map::entry::AVAILABLE) {
			if (!heap->add_free(ent->adr, ent->len))
				return cause::FAIL;
		}

		++ent;
	}

	return cause::OK;
}

/// @brief  前フェーズから使用中のメモリを tmp_alloc から外す。
/// @retval cause::OK   成功した。
/// @retval cause::FAIL メモリの情報が見つからないか、tmp_alloc があふれた。
cause::t load_allocated(tmp_alloc* heap)
{
	const void* src = bootinfo::get_info(bootinfo::TYPE_MEM_ALLOC);
	if (src == 0)
		return cause::FAIL;

	const bootinfo::mem_alloc* ma = (const bootinfo::mem_alloc*)src;
	const void* end = (const u8*)ma + ma->size;
	const bootinfo::mem_alloc_entry* entry = ma->entries;
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
		log()("!!! Available memory detection failed.")();
		return r;
	}

	r = load_allocated(heap);
	if (is_fail(r)) {
		log()("!!! Could not detect alloced memory.")();
		return r;
	}

	return cause::OK;
}

class page_table_tmpacquire
{
	tmp_alloc*                  tmpalloc;
	const tmp_alloc::slot_mask  slotm;

public:
	page_table_tmpacquire(tmp_alloc* _alloc, tmp_alloc::slot_mask _slotm)
	    : tmpalloc(_alloc), slotm(_slotm)
	{}
	cause::pair<uptr> acquire(void*);
	cause::t          release(u64 padr);
};

cause::pair<uptr> page_table_tmpacquire::acquire(void*)
{
	void* p = tmpalloc->alloc(
	    slotm,
	    arch::page::PHYS_L1_SIZE,
	    arch::page::PHYS_L1_SIZE,
	    true);

	return cause::pair<uptr>(
	    p != 0 ? cause::OK : cause::NOMEM,
	    reinterpret_cast<uptr>(p));
}

cause::t page_table_tmpacquire::release(u64 padr)
{
	void* p = reinterpret_cast<void*>(padr);

	return tmpalloc->dealloc(SLOTM_ANY, p) ? cause::OK : cause::FAIL;
}

inline void* pam1_p2v(uptr padr) {
	return reinterpret_cast<void*>(padr);
}
class pam1_page_table_acquire;
typedef arch::page_table<pam1_page_table_acquire, pam1_p2v> _pam1_page_table;
class pam1_page_table : public _pam1_page_table
{
public:
	pam1_page_table(arch::pte* top, tmp_alloc* _heap) :
		_pam1_page_table(top), heap(_heap)
	{}

	tmp_alloc* heap;
};
class pam1_page_table_acquire
{
public:
	static cause::pair<uptr> acquire(_pam1_page_table* x);
};
cause::pair<uptr> pam1_page_table_acquire::acquire(_pam1_page_table* x)
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

inline void* pam2_p2v(u64 padr) {
	return reinterpret_cast<void*>(arch::PHYS_MAP_ADR + padr);
}
class pam2_page_table_acquire;
typedef arch::page_table<pam2_page_table_acquire, pam2_p2v> _pam2_page_table;
class pam2_page_table : public _pam2_page_table
{
public:
	pam2_page_table(arch::pte* top, tmp_alloc* _heap) :
		_pam2_page_table(top), heap(_heap)
	{}

	tmp_alloc* heap;
};
class pam2_page_table_acquire
{
public:
	static cause::pair<uptr> acquire(_pam2_page_table* x);
};
cause::pair<uptr> pam2_page_table_acquire::acquire(_pam2_page_table* x)
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

/// @brief ACPI のインタフェースで CPU を数える。
cause::t count_cpus_by_acpi(tmp_alloc* heap, cpu_id* cpucnt)
{
#if CONFIG_ACPI
	void* acpi_buffer = heap->alloc(SLOTM_ANY,
	                                arch::page::L1_SIZE,  // size
	                                arch::page::L1_SIZE,  // align
	                                true);                // forget
	if (!acpi_buffer)
		return cause::NOMEM;

	cause::t r = acpi_table_init(
	    arch::page::L1_SIZE,
	    arch::map_phys_adr(acpi_buffer, arch::page::L1_SIZE));
	if (is_fail(r))
		return r;

#else  // CONFIG_ACPI
	return cause::FAIL;

#endif  // CONFIG_ACPI
	return cause::FAIL;
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

/// @return mpspec の情報からCPUの数を数えて返す。
int count_cpus(const mpspec& mps)
{
	int cpu_num = 0;

	mpspec::processor_iterator proc_itr(&mps);
	for (;;) {
		const mpspec::processor_entry* pe = proc_itr.get_next();
		if (!pe)
			break;
		++cpu_num;
	}

	return cpu_num;
}

/// @brief メモリを切り出す。
//
/// *ar の範囲をページアライメントに合わせて両端を切り落とし、
/// それでも assign_bytes より大きければ、あまりを切り落とす。
/// @retval true  成功した。
/// @retval false 割り当てられるページが無いため失敗した。
///               この場合は *ar には戻り値が設定されない。
bool cut_ram(uptr assign_bytes, adr_range* ar)
{
	const uptr page_size = arch::page::PHYS_L1_SIZE;

	const uptr low = up_align(ar->low_adr(), page_size);
	if (low < ar->low_adr())
		return false;

	const uptr high = down_align(ar->high_adr() + 1, page_size) - 1;
	if (high > ar->high_adr())
		return false;

	const uptr bytes = high - low + 1;
	if (bytes < page_size)
		return false;

	ar->set_ab(low, min(bytes, assign_bytes));

	return true;
}

/// @brief avail_ram から node_ram へメモリを移す
//
/// avail_ram から *assign_bytes を上限として node_ram へ移す。
/// node_ram へ移されたメモリは avail_ram から除外し、移したメモリサイズを
/// *assign_bytes から減算する。
/// @retval true  成功した。
/// @retval false 失敗した。
/// @note この関数が失敗する状況からソフトで復旧するのは難しい。
bool assign_ram_piece(
    uptr* assign_bytes,
    tmp_alloc* avail_ram,
    tmp_alloc* node_ram)
{
	tmp_alloc::enum_desc ed;

	avail_ram->enum_free(1 << 0, &ed);

	adr_range ar;
	if (!avail_ram->enum_free_next(&ed, &ar))
		return false;

	if (!cut_ram(*assign_bytes, &ar)) {
		// 半端なページを捨てる
		if (!avail_ram->reserve(1 << 0, ar, true))
			return false;

		return true;
	}

	if (!node_ram->add_free(0, ar))
		return false;

	*assign_bytes -= ar.bytes();

	if (!avail_ram->reserve(1 << 0, ar, true))
		return false;

	return true;
}

bool assign_ram(uptr assign_bytes, tmp_alloc* avail_ram, tmp_alloc* node_ram)
{
	do {
		if (!assign_ram_piece(&assign_bytes, avail_ram, node_ram))
			return false;
	} while (assign_bytes >= arch::page::L1_SIZE);

	return true;
}

cause::t _proj_free_mem(
    uptr free_adr,               ///< [in] 空きメモリのアドレス
    uptr free_bytes,             ///< [in] 空きメモリのサイズ
    const tmp_alloc* node_ram,   ///< [in] ノードに割り当てられたメモリ情報
          tmp_alloc* node_heap)  ///< [in,out] メモリ割当先ノードのヒープ
{
	tmp_alloc::enum_desc ed;
	node_ram->enum_free(1 << 0, &ed);

	const uptr free_l = free_adr;
	const uptr free_h = free_adr + free_bytes - 1;

	for (;;) {
		uptr ram_adr, ram_bytes;
		if (!node_ram->enum_free_next(&ed, &ram_adr, &ram_bytes))
			break;

		const uptr ram_l = ram_adr;
		const uptr ram_h = ram_adr + ram_bytes - 1;
		uptr add_l, add_h;

		if (test_overlap(ram_l, ram_h, free_l, free_h, &add_l, &add_h))
		{
			const uptr add_adr = add_l;
			const uptr add_bytes = add_h - add_l + 1;
			if (!node_heap->add_free(0, add_adr, add_bytes))
				return cause::FAIL;
		}
	}

	return cause::OK;
}

/// @brief ノードに割り当てられたメモリ中の空きメモリを node_heap へ追加する。
//
/// @retval cause::OK    成功した。
/// @retval cause::FAIL  node_heap のエントリがあふれた。
cause::t proj_free_mem(
    const tmp_alloc* heap,       ///< [in] 空きメモリ情報
    const tmp_alloc* node_ram,   ///< [in] ノードに割り当てられたメモリ情報
          tmp_alloc* node_heap)  ///< [in,out] メモリ割当先ノードのヒープ
{
	tmp_alloc::enum_desc ed;
	heap->enum_free(SLOTM_ANY, &ed);

	for (;;) {
		uptr adr, bytes;
		const bool end = heap->enum_free_next(&ed, &adr, &bytes);
		if (!end)
			break;

		const cause::t r =
		    _proj_free_mem(adr, bytes, node_ram, node_heap);
		if (is_fail(r))
			return r;
	}

	return cause::OK;
}

cause::t load_page_pool(
    const tmp_alloc* node_ram,
    tmp_alloc* node_heap,
    page_pool* pp)
{
	tmp_alloc::enum_desc ed;
	node_ram->enum_free(1 << 0, &ed);

	for (;;) {
		uptr adr, bytes;
		if (!node_ram->enum_free_next(&ed, &adr, &bytes))
			break;

		cause::t r = pp->add_range(adr_range::gen_ab(adr, bytes));
		if (is_fail(r))
			return r;
	}

	uptr workarea_bytes = pp->calc_workarea_bytes();
	void* workarea_mem = node_heap->alloc(
	    1 << 0, workarea_bytes, arch::BASIC_TYPE_ALIGN, false);
	if (!pp->init(workarea_bytes,
	              arch::map_phys_adr(workarea_mem, workarea_bytes)))
		return cause::FAIL;

	node_heap->enum_free(1 << 0, &ed);
	for (;;) {
		uptr adr, bytes;
		if (!node_heap->enum_free_next(&ed, &adr, &bytes))
			break;

		pp->load_free_range(adr, bytes);
	}

	pp->build();

	return cause::OK;
}

/// @brief  page_pool から1ページを割り当てて native_cpu_node を作る。
/// @retval r=cause::OK Succeeds, value is pointer to native_cpu_node.
cause::pair<x86::native_cpu_node*> create_native_cpu_node(page_pool* pp)
{
	typedef cause::pair<x86::native_cpu_node*> ret_type;

	// native_cpu_node はCPUごとに別ページになるように１ページを
	// 固定で割り当てる。
	// native_cpu_node のサイズが１ページを超えたらソースレベルで
	// 何とかする。
	if (sizeof (x86::native_cpu_buffer) <= arch::page::PHYS_L1_SIZE) {
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

	x86::native_cpu_buffer* cpubuf = new (buf) x86::native_cpu_buffer;

	return ret_type(cause::OK, &cpubuf->node);
}

void set_page_pool_to_cpu_node(cpu_id cpu_node_id)
{
	page_pool** const pps = global_vars::core.page_pool_objs;

	cpu_node* cn = global_vars::core.cpu_node_objs[cpu_node_id];

	const int n = global_vars::core.page_pool_cnt;
	cn->set_page_pool_cnt(n);

	int pp_id = cpu_node_id;
	for (int i = 0; i < n; ++i) {
		cn->set_page_pool(i, pps[pp_id]);
		if (++pp_id >= n)
			pp_id = 0;
	}
}

cause::t create_cpu_nodes()
{
	const int n = global_vars::core.cpu_node_cnt;
	for (int i = 0; i < n; ++i) {
		page_pool* pp = global_vars::core.page_pool_objs[i];
		auto rcn = create_native_cpu_node(pp);
		if (is_fail(rcn))
			return rcn.r;
		global_vars::core.cpu_node_objs[i] = rcn.value;

		set_page_pool_to_cpu_node(i);
	}

	return cause::OK;
}

/// @brief  Local APIC ID を振りなおす。
//
/// Local APIC ID が 0 から始まる連番になるようにする。
cause::t renumber_cpu_ids(const mpspec& mps)
{
	const int cpu_cnt = get_cpu_node_count();
	cpu_node** const cns = global_vars::core.cpu_node_objs;

	int left = cpu_cnt;

	// ID が cpu_cnt 以下の CPU は ID を変更せずに登録する。
	for (mpspec::processor_iterator proc_itr(&mps); ; ) {
		const mpspec::processor_entry* pe = proc_itr.get_next();
		if (!pe)
			break;

		const u8 id = pe->localapic_id;
		if (id < cpu_cnt) {
			static_cast<x86::native_cpu_node*>(cns[id])->set_original_lapic_id(id);
			--left;
		}
	}

	int cns_i = 0;
	for (mpspec::processor_iterator proc_itr(&mps); ; ) {
		const mpspec::processor_entry* pe = proc_itr.get_next();
		if (!pe)
			break;

		const u8 id = pe->localapic_id;
		if (id >= cpu_cnt) {
			while (static_cast<x86::native_cpu_node*>(cns[cns_i])->original_lapic_id_is_enable())
				++cns_i;
			static_cast<x86::native_cpu_node*>(cns[cns_i])->set_original_lapic_id(id);
			--left;
		}
	}

	if (left != 0)
		return cause::FAIL;

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

	cpu_id cpucnt;
	r = count_cpus_by_acpi(&heap, &cpucnt);

	// PAM1 が利用可能になれば mpspec をロード可能になる。
	mpspec mps;
	r = mps.load();
	if (is_fail(r)) {
		// TODO: mpspec が無い場合は単一CPUとして処理する。
		return r;
	}

	global_vars::core.cpu_node_cnt = min(count_cpus(mps), CONFIG_MAX_CPUS);
	global_vars::core.page_pool_cnt = global_vars::core.cpu_node_cnt;

	// init gvar
	for (int i = 0; i < CONFIG_MAX_CPUS; ++i)
		global_vars::core.cpu_node_objs[i] = 0;

	const uptr page_pool_objs_sz =
	    sizeof (page_pool*) * global_vars::core.cpu_node_cnt;
	global_vars::core.page_pool_objs =
	    new (arch::map_phys_adr(heap.alloc(
	        SLOTM_ANY,
	        page_pool_objs_sz,
	        arch::BASIC_TYPE_ALIGN,
	        false), page_pool_objs_sz)
	    ) page_pool*[global_vars::core.cpu_node_cnt];

	tmp_alloc* avail_ram = new (allocate_tmp_alloc(&heap)) tmp_alloc;
	if (!avail_ram)
		return cause::NOMEM;

	tmp_separator sep(avail_ram);
	sep.set_slot_range(0, 0, UPTR_MAX);
	r = load_avail(&sep);
	if (is_fail(r)) {
		log()("!!! Available memory detection failed.")();
		return r;
	}

	// TODO:free
	void* node_ram_mem = allocate_tmp_alloc(&heap);
	void* node_heap_mem = allocate_tmp_alloc(&heap);

	for (int i = 0; i < global_vars::core.page_pool_cnt; ++i) {
		// 未割り当てのメモリサイズを未割り当てのCPU数で割る
		const uptr assign_bytes =
		    avail_ram->total_free_bytes(1 << 0) /
		    (global_vars::core.cpu_node_cnt - i);

		tmp_alloc* node_ram = new (node_ram_mem) tmp_alloc;
		tmp_alloc* node_heap = new (node_heap_mem) tmp_alloc;

		if (!assign_ram(assign_bytes, avail_ram, node_ram))
			log()("!! Page assign to node failed.")();
		proj_free_mem(&heap, node_ram, node_heap);

		void* pp_mem = node_heap->alloc(1 << 0, sizeof (page_pool), arch::BASIC_TYPE_ALIGN, false);
		page_pool* pp = new (arch::map_phys_adr(pp_mem, sizeof (page_pool))) page_pool;
		global_vars::core.page_pool_objs[i] = pp;

		r = load_page_pool(node_ram, node_heap, pp);
		if (is_fail(r))
			return r;

		node_ram->~tmp_alloc();
		node_heap->~tmp_alloc();
		operator delete(node_ram, node_ram_mem);
		operator delete(node_heap, node_heap_mem);
	}

	r = create_cpu_nodes();
	if (is_fail(r))
		return r;

	r = renumber_cpu_ids(mps);
	if (is_fail(r))
		return r;

	page_heap_dealloc(node_ram_mem);
	page_heap_dealloc(node_heap_mem);

	return cause::OK;
}

