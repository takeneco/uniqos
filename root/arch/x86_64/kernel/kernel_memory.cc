/// @file  kernel_memory.cc
/// @brief Kernel internal virtual memory management.
//
// (C) 2010 KATO Takeshi
//

#include "arch.hh"
#include "core_class.hh"
#include "global_vars.hh"
#include "memory_allocate.hh"
#include "placement_new.hh"


using namespace arch::kmem;


// page_header

page_header::page_header(u32 page_size_, allocatable_page* status_)
    : page_size(page_size_), free_chain(), status(status_)
{
	free_piece_header* tmp =
	    new (this + 1) free_piece_header(page_size_ - sizeof *this);
	free_chain.insert_head(tmp);
}

/// @brief ページ内の空きメモリから size より大きいものを探す。
/// @param[in] bytes 探しているメモリサイズ
/// @return 発見したメモリの free_piece_header ポインタを返す。
//
/// ここで最適なサイズの空きメモリを探すくらいのことはしたい。
free_piece_header* page_header::search_free_piece(uptr bytes)
{
	for (free_piece_header* p = free_chain.get_head();
	     p != 0;
	     p = free_chain.get_next(p))
	{
		if (p->piece_bytes >= bytes)
			return p;
	}

	return 0;
}

/// @brief 最大空き piece のサイズを返す。
u32 page_header::search_max_free_bytes() const
{
	u32 max_bytes = 0;
	for (const free_piece_header* p = free_chain.get_head();
	     p != 0;
	     p = free_chain.get_next(p))
	{
		if (p->piece_bytes > max_bytes)
			max_bytes = p->piece_bytes;
	}

	return max_bytes;
}

/// hold_piece を free_piece にする。
cause::stype page_header::set_free(hold_piece_header* piece)
{
	uptr piece_adr = reinterpret_cast<uptr>(piece);

	// 直前の piece が free_piece なら結合する。
	free_piece_header* free_piece;
	for (free_piece = free_chain.get_head();
	         reinterpret_cast<uptr>(free_piece) < piece_adr &&
		 free_piece != 0;
	     free_piece = free_chain.get_next(free_piece))
	{
		if (free_piece->get_after_piece_adr() == piece_adr) {
			free_piece->combine(piece);
			piece = 0;
			break;
		}
	}

	// 直前の piece が free_piece でなければ、
	// hold_piece を free_piece に変える。
	if (piece != 0) {
		free_piece_header* tmp = free_piece;
		const u32 bytes = piece->get_bytes();
		free_piece = new (piece) free_piece_header(bytes);
		if (tmp != 0)
			free_chain.insert_prev(tmp, free_piece);
		else
			free_chain.insert_tail(free_piece);
	}

	// 直後の piece が free_piece なら結合する。
	// ここで remove from list
	free_piece_header* free_after = free_chain.get_next(free_piece);
	if (reinterpret_cast<uptr>(free_after) ==
	        free_piece->get_after_piece_adr())
	{
		free_chain.remove(free_after);
		free_piece->combine(free_after);
	}

	status->growed_free_piece(free_piece->get_bytes());

	return cause::OK;
}


// allocatable_page

void allocatable_page::init(uptr page_adr_, u32 page_size)
{
	page_adr = page_adr_;
	max_free_bytes = page_size - sizeof (page_header);

	new (reinterpret_cast<void*>(page_adr_)) page_header(page_size, this);
}

/// @brief ページの中からメモリを割り当てる。
/// @param[in] bytes 割り当てるメモリのサイズ
/// @return 割り当てられたメモリの hold_piece_header のポインタを返す。
hold_piece_header* allocatable_page::alloc(uptr bytes)
{
	page_header* page = get_page_header();

	bytes = up_align<uptr>(
	    bytes + sizeof (hold_piece_header), arch::BASIC_TYPE_ALIGN);

	free_piece_header* free_piece = page->search_free_piece(bytes);
	if (free_piece == 0)
		return 0;

	bool recalc_max_free = false;
	if (free_piece->piece_bytes >= max_free_bytes) {
		// 一番大きな空きメモリを削ることになるので、
		// あとで max_free_bytes を更新する。
		recalc_max_free = true;
	}

	void* hold_ptr;
	const uptr rest_bytes = free_piece->piece_bytes - bytes;
	if (rest_bytes < sizeof (free_piece_header)) {
		// 発見したピースを丸ごと確保する。
		bytes = free_piece->piece_bytes;
		page->remove_free(free_piece);
		hold_ptr = free_piece;
	} else {
		// 発見したピースのシッポからメモリを削りだす。
		hold_ptr = free_piece->cut(bytes);
	}

	hold_piece_header* ret =
	    new (hold_ptr) hold_piece_header(page, bytes);

	if (recalc_max_free)
		max_free_bytes = page->search_max_free_bytes();

	return ret;
}


// allocatable_page_array

/// @brief 空いている allocatable_page のアドレスを返す。
/// @return allocatable_page の空きがないときは 0 を返す。
allocatable_page* allocatable_page_array::new_entry()
{
	if (info.page_num == ARRAY_LENGTH)
		return 0;

	for (int i = 0; i < ARRAY_LENGTH; ++i) {
		if (!page_array[i].is_captured())
			return &page_array[i];
	}

	return 0;
}

/// @新しいページを管理下に加える。
/// @param[in] padr 新しいページの物理アドレス。
allocatable_page* allocatable_page_array::add_page(uptr page_adr, u32 page_size)
{
	allocatable_page* entry = new_entry();
	if (entry == 0)
		return 0;

	++info.page_num;

	entry->init(page_adr, page_size);

	return entry;
}

hold_piece_header* allocatable_page_array::alloc(uptr bytes)
{
	// メモリを確保するときは、hold_piece_header をヘッダにするので、
	// その分を足したサイズで空きメモリを探す。
	const uptr bytes_with_header = up_align<uptr>(
	    bytes + sizeof (hold_piece_header), arch::BASIC_TYPE_ALIGN);

	const int page_num = info.page_num;

	int pages = 0;
	for (int i = 0; i < ARRAY_LENGTH; ++i) {
		if (pages >= page_num)
			break;

		if (!page_array[i].is_captured())
			continue;

		const u32 max_free_bytes = page_array[i].get_max_free_bytes();

		// 最初に見つけたページからメモリを割り当てる。
		if (max_free_bytes >= bytes_with_header)
			return page_array[i].alloc(bytes);

		++pages;
	}

	// 空きメモリがあることを確認してから呼ばれるので、
	// ここには到達しないはず。
	return 0;
}


// kernel_memory

/// @brief カーネルメモリとして予約済みのページからメモリを割り当てる。
/// @param[in] size 割り当てるメモリサイズ。
/// @return 割り当てられたメモリのヘッダを返す。
/// @return 割り当てられない場合は 0 を返す。
hold_piece_header* kernel_memory::alloc_from_existpage(uptr size)
{
	for (allocatable_page_array* ary = allocatable_chain.get_head();
	    ary != 0;
	    ary = allocatable_chain.get_next(ary))
	{
		hold_piece_header* p = ary->alloc(size);
		if (p != 0)
			return p;
	}

	return 0;
}

/// @brief 新しいページからメモリを割り当てる。
/// @param[in] size 割り当てるメモリサイズ。
/// @return 割り当てられたメモリのヘッダを返す。
/// @return 割り当てられない場合は 0 を返す。
hold_piece_header* kernel_memory::alloc_from_newpage(uptr size)
{
	const uptr header_size =
	    sizeof (page_header) + sizeof (hold_piece_header);

	uptr page_adr;
	u32 page_size;
	cause::stype r;
	if (size <= (arch::page::PHYS_L1_SIZE - header_size)) {
		page_size = arch::page::PHYS_L1_SIZE;
		r = arch::page::alloc(arch::page::L1, &page_adr);
	} else if (size <= (arch::page::PHYS_L2_SIZE - header_size)) {
		page_size = arch::page::PHYS_L2_SIZE;
		r = arch::page::alloc(arch::page::L2, &page_adr);
	} else {
		r = cause::NO_IMPLEMENTS;
	}
	if (r != cause::OK) {
		// TODO: This is error
		return 0;
	}

	page_adr += arch::PHYSICAL_MEMMAP_BASEADR;

	allocatable_page* page = 0;
	for (allocatable_page_array* ary = allocatable_chain.get_head();
	     ary != 0;
	     ary = allocatable_chain.get_next(ary))
	{
		page = ary->add_page(page_adr, page_size);
		if (page)
			break;
	}

	if (page == 0) {
		// allocatable_page_array を追加する。
		uptr tmp;
		r = arch::page::alloc(arch::page::L1, &tmp);
		if (r != cause::OK) {
			page_size == arch::page::PHYS_L1_SIZE ?
			    arch::page::free(arch::page::L1, page_adr) :
			    arch::page::free(arch::page::L2, page_adr);
			return 0;
		}
		allocatable_page_array* ary =
		    new (reinterpret_cast<void*>(
		        tmp + arch::PHYSICAL_MEMMAP_BASEADR))
		    allocatable_page_array;
		page = ary->add_page(page_adr, page_size);
		if (page == 0) {
			// TODO: This is error
		}

		allocatable_chain.insert_tail(ary);
	}

	return page->alloc(size);
}

void* kernel_memory::alloc(uptr size)
{
	hold_piece_header* p = alloc_from_existpage(size);

	if (p == 0) {
		p = alloc_from_newpage(size);
		if (p == 0)
			return 0;
	}

	return p->get_memory_ptr();
}

cause::stype kernel_memory::free(void* ptr)
{
	hold_piece_header* piece = hold_piece_header::ptr_to_hold_piece(ptr);
	page_header* page = piece->get_page_header();

	return page->set_free(piece);
}


namespace memory
{

cause::stype init()
{
	uptr p;
	cause::stype r = arch::page::alloc(arch::page::L1, &p);
	if (r != cause::OK)
		return r;

	global_vars::gv.core =
	    new (arch::map_phys_mem(p, sizeof (core_class)))
	    core_class;

	return cause::OK;
}

void* alloc(uptr bytes)
{
	return global_vars::gv.core->kmem_ctrl.alloc(bytes);
}

cause::stype free(void* ptr)
{
	return global_vars::gv.core->kmem_ctrl.free(ptr);
}

}

