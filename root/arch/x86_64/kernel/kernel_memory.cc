/// @file  kernel_memory.cc
/// @brief Kernel internal virtual memory management.
//
// (C) 2010 KATO Takeshi
//

#include "arch.hh"
#include "btypes.hh"
#include "chain.hh"
#include "global_variables.hh"
#include "physical_memory.hh"
#include "placement_new.hh"


namespace {

class allocatable_page;

/// ページの先頭にページヘッダ(page_header)を置く。
/// ページをピース(piece)に切り出してメモリを割り当てる。
/// 割当済みピースの先頭にはresv_piece_headerを置く。
/// 空きピースの先頭にはfree_piece_headerを置く。

/// @brief 割当済みピースのヘッダ
class hold_piece_header
{
	/// ヘッダも含めたサイズ
	u32 piece_bytes;
	/// ページ先頭への相対ポインタ。
	/// this = &page_header + page_offset になる
	u32 page_offset;

public:
	hold_piece_header(page_header* page_header_, u32 piece_bytes_) :
	    piece_bytes(piece_bytes_),
	{
		page_offset = reinterpret_cast<u32>(
		    reinterpret_cast<uptr>(this) -
		    reinterpret_cast<uptr>(page_header_)
		);
	}
	void* get_memory_ptr() {
		return this + 1;
	}
};


/// @brief 空きピースのヘッダ
class free_piece_header
{
	/// ヘッダも含めたサイズ
	u32  piece_bytes;
	int  reserved; // for lock

public:
	bichain_link<free_piece_header> chain_link_;

	free_piece_header() {}

	/// @brief メモリを切り出す。
	//
	/// メモリを後ろから切り取って、切り取ったメモリのポインタを返す。
	void* cut(u32 bytes) {
		piece_bytes -= bytes;
		return reinterpret_cast<u8>(this) + piece_bytes;
	}
};


/// @brief ページ内のメモリ割当状況を管理する。
class page_header
{
	u32  page_size;
	int  reserved;
	bichain<free_piece_header, &free_piece_header::chain_link_> free_chain;
	allocatable_page* status;

public:
	page_header(u32 page_size_, allocatable_page* status_)
	    : page_size(page_size_), status(status_)
	    {}

	free_piece_header* search_free_piece(uptr bytes);
	u32 search_max_free_bytes() const;

	void remove_free(free_piece_header* piece) {
		free_chain.remove(piece);
	}
};

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
	for (free_piece_header* p = free_chain.get_head();
	     p != 0;
	     p = free_chain.get_next(p))
	{
		if (p->piece_bytes > max_bytes)
			max_bytes = p->piece_bytes;
	}

	return max_bytes;
}


/// @brief 割当可能な空きメモリを含むページを管理する。
//
/// 空きメモリを検索するときに、空きメモリのサイズの部分が
/// キャッシュに乗るようにする。
class allocatable_page
{
	/// ページが含む最も大きい空きメモリサイズ。
	u32  max_free_bytes;
	int  dummy___;
	/// ページの先頭アドレス。
	uptr page_adr;

	enum { NOT_CAPTURED = 0xffffffff };

public:
	allocatable_page() : max_free_bytes(NOT_CAPTURED) {}

	bool is_captured() const {
		return max_free_bytes != NOT_CAPTURED;
	}
	u32 get_max_free_bytes() const {
		return max_free_bytes;
	}
	page_header* get_page_header() {
		return reinterpret_cast<page_header*>(page_adr);
	}

	hold_piece_header* alloc(uptr bytes);
};

/// @brief ページの中からメモリを割り当てる。
/// @param[in] bytes 割り当てるメモリのサイズ
/// @return 割り当てられたメモリの hold_piece_header のポインタを返す。
hold_piece_header* allocatable_page::alloc(uptr bytes)
{
	page_header* page = get_page_header();

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

	hold_piece_header* hold_piece =
	    new (hold_ptr) hold_piece_header(page, bytes);

	if (recalc_max_free)
		max_free_bytes = page->search_max_free_bytes();

	return hold_piece;
}


/// @brief allocatable_page の配列。
//
/// チェインでつなぐ。
class allocatable_page_array
{
	// sizeof info と書けるようにまとめた。
	struct array_info
	{
		bichain_link<allocatable_page_array> chain_link_;
		// page_array に含まれる割当可能ページの数
		int page_num;
		// 配列の先頭に最も近い空きエントリのインデックス
		int first_blank_entry;

		array_info()
		    : page_num(0), first_blank_entry(0)
		    {}
	} info;

	// データ型のサイズがページサイズと等しくなるようにする。
	enum {
		ARRAY_LENGTH =
		    (PAGE_L1_SIZE - sizeof info) / sizeof (allocatable_page)
	};
	allocatable_page page_array[ARRAY_LENGTH];

public:
	allocatable_page_array() {}

	allocatable_page* new_entry();
	hold_piece_header* alloc(uptr bytes);
};

/// @brief 空いている allocatable_page のアドレスを返す。
//
/// @return allocatable_page の空きがないときは 0 を返す。
allocatable_page* allocatable_page_array::new_entry()
{
	if (info.page_num == ARRAY_LENGTH)
		return 0;

	for (int i = 0; i < ARRAY_LENGH; ++i) {
		if (!page_array[i].is_captured())
			return &page_array[i];
	}

	return 0;
}

hold_piece_header* allocatable_page_array::alloc(uptr bytes)
{
	// メモリを確保するときは、hold_piece_header をヘッダにするので、
	// その分を足したサイズで空きメモリを探す。
	bytes = up_align(
	    bytes + sizeof (hold_piece_header), BASIC_TYPE_ALIGN);

	const int page_num = info.page_num;

	int pages = 0;
	for (int i = 0; i < ARRAY_LENGTH; ++i) {
		if (pages >= page_num)
			break;

		if (!page_array[i].is_captured())
			continue;

		const u32 max_free_bytes = page_array[i].get_max_free_bytes();

		// 最初に見つけたページからメモリを割り当てる。
		if (max_free_bytes >= bytes)
			return page_array[i].alloc(bytes);

		++pages;
	}

	// 空きメモリがあることを確認してから呼ばれるので、
	// ここには到達しないはず。
	return 0;
}


/// @brief カーネル自身が使うメモリを管理する。
//
/// このクラスは物理メモリ１ページに収まらなければならない。
/// (初期化処理で物理ページを割り当てているだけなので)
class kernel_memory_
{
	bidechain<
	    allocatable_page_array,
	    &allocatable_page_array::info::chain_link_>
	        allocatable_chain;

private:
	void* alloc_from_existpage(uptr size);
	void* alloc_from_newpage(uptr size);

public:
	void* alloc(uptr size);
};

void* kernel_memory_::alloc_from_existpage(uptr size)
{
	for (allocatable_page_array* ary = allocatable_chain.get_head();
	    ary != 0;
	    ary = allocatable_chain.get_next())
	{
		ary->alloc(size);
	}

	return 0;
}

void* kernel_memory_::alloc_from_newpage(uptr size)
{
	const uptr header_size = sizeof page_header + sizeof resv_piece_header;

	uptr padr;
	u32 page_size;
	cause::stype r;
	if (size <= (PAGE_L1_SIZE - header_size)) {
		page_size = PAGE_L1_SIZE;
		r = arch::pmem::alloc_l1page(&padr);
	} else if (size <= (PAGE_L2_SIZE - header_size)) {
		page_size = PAGE_L2_SIZE;
		r = arch::pmem::alloc_l2page(&padr);
	} else {
		r = cause::NOT_IMPLEMENTS;
	}
	if (r != cause::OK) {
		/* TODO: error */
		return 0;
	}

	new (reinterpret_cast<void*>(padr)) page_header(page_size, );
}

void* kernel_memory_::alloc(uptr size)
{
	hold_piece_header* p = alloc_from_existpage(size);
	if (p == 0)
		return 0;

	return p->get_memory_ptr();
}

}  // namespace


class kernel_memory : public kernel_memory_
{
};


namespace memory
{

cause::stype init()
{
	uptr p;
	cause::stype r = arch::pmem::alloc_1page(&p);
	if (r != cause::OK)
		return r;

	global_variable::gv.kmem_ctrl =
	     new (reinterpret_cast<void*>(PHYSICAL_MEMMAP_BASEADR + p))
	     kernel_memory;
}

void* alloc(uptr size)
{
	return global_variable::gv.kmem_ctrl->alloc(size);
}

void free(void* ptr)
{
}

}

