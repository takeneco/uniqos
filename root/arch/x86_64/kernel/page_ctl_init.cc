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
#include "bootinfo.hh"
#include "cheap_alloc.hh"
#include "gv_page.hh"
#include "log.hh"
#include <mpspec.hh>
#include "native_ops.hh"
#include "page_ctl.hh"
#include "pagetable.hh"
#include <processor.hh>
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

inline arch::page_ctl* get_page_ctl() {
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

/// @brief AVAILABLE とされているメモリのバイト数を返す。
u64 total_avail_pmem()
{
	const void* src = get_bootinfo(MULTIBOOT_TAG_TYPE_MMAP);
	if (src == 0)
		return 0;

	const multiboot_tag_mmap* mmap = (const multiboot_tag_mmap*)src;

	const void* mmap_end = (const u8*)mmap + mmap->size;
	const multiboot_mmap_entry* entry = mmap->entries;

	u64 total = 0;
	while (entry < mmap_end) {
		if (entry->type == MULTIBOOT_MEMORY_AVAILABLE)
			total += entry->len;

		entry = (const multiboot_mmap_entry*)
		    ((const u8*)entry + mmap->entry_size);
	}

	return total;
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

	return p != 0 ? cause::OK : cause::NOMEM;
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

cause::stype load_page(const tmp_alloc& alloc, arch::page_ctl* ctl)
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

cpu_node* create_processor(u8 lapic_id, tmp_alloc* heap)
{
	void* buf = heap->alloc(
	    SLOTM_BOOTHEAP | SLOTM_PAM1,
	    arch::page::PHYS_L1_SIZE,
	    arch::page::PHYS_L1_SIZE,
	    false);
	if (!buf)
		return 0;

	buf = arch::map_phys_adr(buf, arch::page::PHYS_L1_SIZE);

	cpu_node* proc = new (buf) cpu_node;
	proc->set_original_lapic_id(lapic_id);

	return proc;
}

/// @brief processor_objs を作成する。
cause::stype create_processor_objs(
    const mpspec& mps, /// [in] CPUの情報を含んだ mpspec
    tmp_alloc* heap)   /// [in] ヒープ
{
	const int cpu_cnt = global_vars::gv.cpu_node_cnt;

	cpu_node** const proc_ary = global_vars::gv.cpu_node_objs;

	// cpu_node はCPUごとに別ページになるように１ページを固定で割り当てる。
	// cpu_node のサイズが１ページを超えたら何とかする。
	if (sizeof (cpu_node) >= arch::page::PHYS_L1_SIZE)
		log()("!!! sizeof (processor) is too large.")();

	// ID が cpu_cnt 以下の CPU は ID を変更せずに登録する。

	int detected = 0;
	for (mpspec::processor_iterator proc_itr(&mps); ; )
	{
		const mpspec::processor_entry* pe = proc_itr.get_next();
		if (!pe)
			break;

		const u8 id = pe->localapic_id;
		if (id < cpu_cnt)
		// && global_vars::gv.processor_objs[pe->localapic_id] == 0)
		{
			proc_ary[id] = create_processor(id, heap);
			if (!proc_ary[id]) {
				log()("!!! No enough memory.")();
				return cause::NOMEM;
			}

			++detected;
		}
	}

	// TODO: local apic ID が跳んでいる場合の処理

	return cause::OK;
}

cause::stype init_page_ctl(arch::page_ctl* pgctl, tmp_alloc* heap)
{
	const uptr pmem_end = search_pmem_end();
	const uptr buf_bytes = pgctl->calc_workarea_size(pmem_end);

	void* buf = heap->alloc(
	    SLOTM_BOOTHEAP | SLOTM_PAM1,
	    buf_bytes,
	    arch::page::PHYS_L1_SIZE,
	    false);

	if (!buf) {
		log(1)("No enough memory");
		return cause::NOMEM;
	}
	buf = arch::map_phys_adr(buf, buf_bytes);

	pgctl->init(pmem_end, buf);
	return cause::OK;
}

bool assign_mem_piece(
    tmp_alloc* avail_mem,
    uptr* assign_bytes,
    arch::page_ctl* pgctl)
{
	tmp_alloc::enum_desc ed;

	avail_mem->enum_free(1 << 0, &ed);

	uptr adr, bytes;
	const bool r = avail_mem->enum_free_next(&ed, &adr, &bytes);
	if (!r)
		return false;

	if (bytes > *assign_bytes) {
		uptr endadr = adr + *assign_bytes;
		endadr = down_align<uptr>(endadr, arch::page::PHYS_L1_SIZE);
		if (endadr < adr) {
			// TODO
		}
		bytes = endadr - adr;
	}

	log(1)("assign: ").u(adr, 16)(" size:").u(bytes, 16)();

	pgctl->load_free_range(adr, bytes);
	*assign_bytes -= bytes;

	avail_mem->reserve(1 << 0, adr, bytes, true);

	return true;
}

void assign_mem(tmp_alloc* mem, int cpu)
{
	arch::page_ctl& pgctl =
	    global_vars::gv.cpu_node_objs[cpu]->get_page_ctl();

	// 未割り当てのメモリサイズを未割り当てのCPU数で割る
	uptr assign_bytes = mem->total_free_bytes(1 << 0) /
	    (global_vars::gv.cpu_node_cnt - cpu);

	do {
		assign_mem_piece(mem, &assign_bytes, &pgctl);
	} while (assign_bytes >= arch::page::L1_SIZE);

	pgctl.build();
}

}  // namespace


cause::stype cpupage_init()
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

	// PAM1 が利用可能になれば mpspec をロード可能になる。
	mpspec mps;
	r = mps.load();
	if (is_fail(r)) {
		// TODO: mpspec が無い場合は単一CPUとして処理する。
		return r;
	}

	global_vars::gv.cpu_node_cnt = count_cpus(mps);

	// init gvar
	for (int i = 0; i < CONFIG_MAX_CPUS; ++i)
		global_vars::gv.cpu_node_objs[i] = 0;

	r = create_processor_objs(mps, &heap);
	if (is_fail(r))
		return r;

	for (int i = 0; i < CONFIG_MAX_CPUS; ++i) {
		log(1)("proc[").u(i)("]: ")(global_vars::gv.cpu_node_objs[i])();
	}

	const uptr total_pmem_bytes = total_avail_pmem();
	log(1)("total_pmem: ").u(total_pmem_bytes, 16)();

	tmp_alloc mem;
	tmp_separator memsep(&mem);
	memsep.set_slot_range(0, 0, U64(0xffffffffffffffff));
	load_avail(&memsep);

	uptr left_pmem_bytes = total_pmem_bytes;
	for (int cpu = 0; cpu < global_vars::gv.cpu_node_cnt; ++cpu) {
		log(1)("cpu : ").u(cpu)();
		arch::page_ctl& pgctl =
		    global_vars::gv.cpu_node_objs[cpu]->get_page_ctl();

		r = init_page_ctl(&pgctl, &heap);
		if (is_fail(r))
			return r;

		assign_mem(&mem, cpu);
	}

	return cause::OK;
}

/// @retval cause::NOMEM  No enough physical memory.
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
		return cause::NOMEM;
	}
	buf = arch::map_phys_adr(buf, arch::page::PHYS_L1_SIZE);
	if (setup_gv_page(buf, arch::page::PHYS_L1_SIZE) == false)
		return cause::UNKNOWN;

	arch::page_ctl* pgctl = get_page_ctl();

	// 物理メモリの終端アドレス。これを物理メモリサイズとする。
	const uptr pmem_end = search_pmem_end();

	buf = heap.alloc(
	    SLOTM_BOOTHEAP,
	    pgctl->calc_workarea_size(pmem_end),
	    arch::page::PHYS_L1_SIZE,
	    false);
	if (!buf) {
		log()("bootheap is exhausted.")();
		return cause::NOMEM;
	}

	pgctl->init(pmem_end, buf);

	r = load_page(heap, pgctl);
	if (r != cause::OK)
		return r;

	pgctl->build();

	return cause::OK;
}

