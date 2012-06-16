/// @file  page_ctl_init.cc
/// @brief Page control initialize.
//
// (C) 2010-2012 KATO Takeshi
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
#include "bootinfo.hh"
#include "cheap_alloc.hh"
#include "gv_page.hh"
#include "log.hh"
#include <mpspec.hh>
#include "native_ops.hh"
#include "page_ctl.hh"
#include <page_pool.hh>
#include "pagetable.hh"
#include <processor.hh>
#include "setupdata.hh"


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

/// @brief  物理的に存在するメモリを調べて tmp_separator に積む。
/// @retval cause::OK   成功した。
/// @retval cause::FAIL メモリの情報が見つからないか、tmp_alloc があふれた。
cause::stype load_avail(tmp_separator* heap)
{
	const void* src = get_bootinfo(MULTIBOOT_TAG_TYPE_MMAP);
	if (src == 0)
		return cause::FAIL;

	const multiboot_tag_mmap* mmap = (const multiboot_tag_mmap*)src;

	const void* mmap_end = (const u8*)mmap + mmap->size;
	const multiboot_mmap_entry* entry = mmap->entries;

	while (entry < mmap_end) {
		if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
			if (!heap->add_free(entry->addr, entry->len))
				return cause::FAIL;
		}

		entry = (const multiboot_mmap_entry*)
		    ((const u8*)entry + mmap->entry_size);
	}

	return cause::OK;
}

/// @brief  前フェーズから使用中のメモリを tmp_alloc から外す。
/// @retval cause::OK   成功した。
/// @retval cause::FAIL メモリの情報が見つからないか、tmp_alloc があふれた。
cause::stype load_allocated(tmp_alloc* heap)
{
	const void* src = get_bootinfo(bootinfo::TYPE_MEMALLOC);
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
cause::stype setup_heap(tmp_alloc* heap)
{
	tmp_separator sep(heap);

	for (u32 i = 0; i < num_of_array(setup_heap_slots); ++i) {
		sep.set_slot_range(
		    setup_heap_slots[i].slot,
		    setup_heap_slots[i].slot_head,
		    setup_heap_slots[i].slot_tail);
	}

	cause::type r = load_avail(&sep);
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

class page_table_tmpalloc
{
	tmp_alloc*                  tmpalloc;
	const tmp_alloc::slot_mask  slotm;

public:
	page_table_tmpalloc(tmp_alloc* _alloc, tmp_alloc::slot_mask _slotm)
	    : tmpalloc(_alloc), slotm(_slotm)
	{}
	cause::stype alloc(u64* padr);
	cause::stype free(u64 padr);
};

cause::stype page_table_tmpalloc::alloc(u64* padr)
{
	void* p = tmpalloc->alloc(
	    slotm,
	    arch::page::PHYS_L1_SIZE,
	    arch::page::PHYS_L1_SIZE,
	    true);

	*padr = reinterpret_cast<u64>(p);

	return p != 0 ? cause::OK : cause::NOMEM;
}

cause::stype page_table_tmpalloc::free(u64 padr)
{
	void* p = reinterpret_cast<void*>(padr);

	return tmpalloc->dealloc(SLOTM_ANY, p) ? cause::OK : cause::FAIL;
}

inline u64 phys_to_virt_1(u64 padr) { return padr; }
inline u64 phys_to_virt_2(u64 padr) { return arch::PHYS_MAP_ADR + padr; }

/// @brief  Setup physical mapping address 1st phase.
/// @param[in] padr_end  Maximum address.
/// @param[in] heap      Memory allocator.
/// @retval true  Succeeds.
/// @retval false Failed.
//
/// BOOTHEAP だけを使い、padr_end が 6GiB 以上の場合でも
/// 最大 6GiB まで作成する。
cause::stype setup_pam1(u64 padr_end, tmp_alloc* heap)
{
	typedef arch::page_table<page_table_tmpalloc, phys_to_virt_1>
	    page_table_1;

	page_table_tmpalloc pt_alloc(heap, SLOTM_BOOTHEAP);

	arch::pte* cr3 = reinterpret_cast<arch::pte*>(native::get_cr3());

	page_table_1 pg_tbl(cr3, &pt_alloc);

	padr_end = min<u64>(padr_end, SETUP_PAM1_MAPEND);

	for (uptr padr = 0; padr < padr_end; padr += arch::page::PHYS_L2_SIZE)
	{
		const cause::stype r = pg_tbl.set_page(
		    arch::PHYS_MAP_ADR + padr,
		    padr,
		    arch::page::PHYS_L2,
		    page_table_1::EXIST | page_table_1::WRITE);

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
cause::stype setup_pam2(u64 padr_end, tmp_alloc* heap)
{
	typedef arch::page_table<page_table_tmpalloc, phys_to_virt_2>
	    page_table_2;

	page_table_tmpalloc pt_alloc(heap, SLOTM_BOOTHEAP | SLOTM_PAM1);

	arch::pte* cr3 = static_cast<arch::pte*>(
	    arch::map_phys_adr(native::get_cr3(), arch::page::PHYS_L1_SIZE));

	page_table_2 pg_tbl(cr3, &pt_alloc);

	for (uptr padr = SETUP_PAM1_MAPEND + 1;
	     padr < padr_end;
	     padr += arch::page::PHYS_L2_SIZE)
	{
		const cause::stype r = pg_tbl.set_page(
		    arch::PHYS_MAP_ADR + padr,
		    padr,
		    arch::page::PHYS_L2,
		    page_table_2::EXIST | page_table_2::WRITE);

		if (is_fail(r))
			return r;
	}

	return cause::OK;
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

bool setup_gv_page(void* page, uptr bytes)
{
	gv_page* gv_page_obj = new (page) gv_page;

	log()("gv_page size: ").u(sizeof *gv_page_obj)();
	if (bytes < sizeof (gv_page)) {
		log()("!!! No enough bytes for gv_page. "
		    "page size=").u(bytes)(
		    ", sizeof gv_page=").u(sizeof (gv_page))();
		return false;
	}

	return gv_page_obj->init();
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

cause::stype _proj_free_mem(
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
cause::stype proj_free_mem(
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

		const cause::stype r =
		    _proj_free_mem(adr, bytes, node_ram, node_heap);
		if (is_fail(r))
			return r;
	}

	return cause::OK;
}

cause::stype load_page_pool(
    const tmp_alloc* node_ram,
    tmp_alloc* node_heap,
    page_pool* pp)
{
	tmp_alloc::enum_desc ed;
	node_ram->enum_free(1 << 0, &ed);

	uptr lower_adr = UPTR_MAX;
	uptr higher_adr = 0;

	for (;;) {
		uptr adr, bytes;
		if (!node_ram->enum_free_next(&ed, &adr, &bytes))
			break;

		lower_adr = min(lower_adr, adr);
		higher_adr = max(higher_adr, adr + bytes - 1);
	}

	pp->set_range(lower_adr, higher_adr);

	uptr workarea_bytes = pp->calc_workarea_bytes();
	void* workarea_mem = node_heap->alloc(
	    1 << 0, workarea_bytes, arch::BASIC_TYPE_ALIGN, false);
	if (!pp->init(workarea_bytes, arch::map_phys_adr(workarea_mem, workarea_bytes)))
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

/// @brief  page_pool から1ページを割り当てて cpu_node を作る。
/// @return  If succeeds, returns ptr to cpu_node.
///          If failed, returns nullptr.
cpu_node* create_cpu_node(page_pool* pp)
{
	// cpu_node はCPUごとに別ページになるように１ページを固定で割り当てる。
	// cpu_node のサイズが１ページを超えたらソースレベルで何とかする。
	if (sizeof (cpu_node) >= arch::page::PHYS_L1_SIZE) {
		log()("!!! sizeof (cpu_node) is too large.")();
		return 0;
	}

	uptr page_adr;
	cause::type r = pp->alloc(arch::page::PHYS_L1, &page_adr);
	if (is_fail(r)) {
		log()("!!! No enough memory.")();
		return 0;
	}

	void* buf = arch::map_phys_adr(page_adr, arch::page::PHYS_L1_SIZE);

	cpu_node* cn = new (buf) cpu_node;

	return cn;
}

void set_page_pool_to_cpu_node(int cpu_node_id)
{
	page_pool** const pps = global_vars::gv.page_pool_objs;

	cpu_node* cn = global_vars::gv.cpu_node_objs[cpu_node_id];

	const int n = global_vars::gv.page_pool_cnt;
	int pp_id = cpu_node_id;
	for (int i = 0; i < n; ++i) {
		cn->set_page_pool(i, pps[pp_id]);
		if (++pp_id >= n)
			pp_id = 0;
	}
}

cause::type create_cpu_nodes()
{
	const int n = global_vars::gv.cpu_node_cnt;
	for (int i = 0; i < n; ++i) {
		page_pool* pp = global_vars::gv.page_pool_objs[i];
		cpu_node* cn = create_cpu_node(pp);
		if (!cn)
			return cause::FAIL;
		global_vars::gv.cpu_node_objs[i] = cn;

		set_page_pool_to_cpu_node(i);
	}

	return cause::OK;
}

/// @brief  Local APIC ID を振りなおす。
//
/// Local APIC ID が 0 から始まる連番になるようにする。
cause::type renumber_cpu_ids(const mpspec& mps)
{
	const int cpu_cnt = get_cpu_node_count();
	cpu_node** const cns = global_vars::gv.cpu_node_objs;

	int left = cpu_cnt;

	// ID が cpu_cnt 以下の CPU は ID を変更せずに登録する。
	for (mpspec::processor_iterator proc_itr(&mps); ; ) {
		const mpspec::processor_entry* pe = proc_itr.get_next();
		if (!pe)
			break;

		const u8 id = pe->localapic_id;
		if (id < cpu_cnt) {
			cns[id]->set_original_lapic_id(id);
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
			while (cns[cns_i]->original_lapic_id_is_enable())
				++cns_i;
			cns[cns_i]->set_original_lapic_id(id);
			--left;
		}
	}

	if (left != 0)
		return cause::FAIL;

	return cause::OK;
}

}  // namespace


cause::stype cpupage_init()
{
	tmp_alloc heap;
	setup_heap(&heap);

	const u64 padr_end = search_padr_end();

	cause::stype r = setup_pam1(padr_end, &heap);
	if (is_fail(r))
		return r;

	r = setup_pam2(padr_end, &heap);
	if (is_fail(r))
		return r;

	// PAM1 が利用可能になれば mpspec をロード可能になる。
	mpspec mps;
	r = mps.load();
	if (is_fail(r)) {
		// TODO: mpspec が無い場合は単一CPUとして処理する。
		return r;
	}

	global_vars::gv.cpu_node_cnt = min(count_cpus(mps), CONFIG_MAX_CPUS);
	global_vars::gv.page_pool_cnt = global_vars::gv.cpu_node_cnt;

	// init gvar
	for (int i = 0; i < CONFIG_MAX_CPUS; ++i)
		global_vars::gv.cpu_node_objs[i] = 0;

	const uptr page_pool_objs_sz =
	    sizeof (page_pool*) * global_vars::gv.cpu_node_cnt;
	global_vars::gv.page_pool_objs =
	    new (arch::map_phys_adr(heap.alloc(
	        SLOTM_ANY,
	        page_pool_objs_sz,
	        arch::BASIC_TYPE_ALIGN,
	        false), page_pool_objs_sz)
	    ) page_pool*[global_vars::gv.cpu_node_cnt];

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

	for (int i = 0; i < global_vars::gv.page_pool_cnt; ++i) {
		// 未割り当てのメモリサイズを未割り当てのCPU数で割る
		const uptr assign_bytes =
		    avail_ram->total_free_bytes(1 << 0) /
		    (global_vars::gv.cpu_node_cnt - i);

		tmp_alloc* node_ram = new (node_ram_mem) tmp_alloc;
		tmp_alloc* node_heap = new (node_heap_mem) tmp_alloc;

		if (!assign_ram(assign_bytes, avail_ram, node_ram))
			log()("!! Page assign to node failed.")();
		proj_free_mem(&heap, node_ram, node_heap);

		void* pp_mem = node_heap->alloc(1 << 0, sizeof (page_pool), arch::BASIC_TYPE_ALIGN, false);
		page_pool* pp = new (arch::map_phys_adr(pp_mem, sizeof (page_pool))) page_pool;
		global_vars::gv.page_pool_objs[i] = pp;

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

	//heap.dealloc(SLOTM_ANY, node_ram_mem);
	//heap.dealloc(SLOTM_ANY, node_heap_mem);

for(;;)native::hlt();

	return cause::OK;
}

/// @retval cause::NOMEM  No enough physical memory.
/// @retval cause::OK  Succeeds.
cause::stype page_ctl_init()
{
	tmp_alloc heap;
	setup_heap(&heap);

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
		return cause::NOMEM;
	}
	buf = arch::map_phys_adr(buf, arch::page::PHYS_L1_SIZE);
	if (setup_gv_page(buf, arch::page::PHYS_L1_SIZE) == false)
		return cause::UNKNOWN;


	return cause::OK;
}

