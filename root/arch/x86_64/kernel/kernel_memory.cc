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


/// @brief 空きメモリのヘッダ
class free_piece_header
{
	u32  piece_size;
	int  reserved; // for lock
public:
	bichain_link<free_piece_header> chain_link_;
};

/// @brief ページ内のメモリ割当状況を管理する。
class page_header
{
	u32  page_size;
	int  reserved;
	bichain<free_piece_header, &free_piece_header::chain_link_> free_chain;
	allocatable_page* status;
};

/// @brief 割当可能な空きメモリを含むページを管理する。
class allocatable_page
{
	/// ページが含む最も大きい空きメモリサイズ。
	u32  max_free_bytes;
	/// ロックに使う予定。
	int  reserved;
	/// ページの先頭アドレス。
	uptr page_adr;

public:
	allocatable_page() : max_free_bytes(0) {}
};

/// @brief allocatable_page の配列。
//
/// チェインでつなぐ。
class allocatable_page_array
{
	// sizeof info と書けるようにまとめた。
	struct array_info
	{
		bichain_link<allocatable_page_array> chain_link_;
		// page_arrya に含まれる割当可能ページの数
		int page_num;
		// ロックに使う予定
		int reserved;

		array_info() : page_num(0) {}
	} info;

	// データ型のサイズがページサイズと等しくなるようにする。
	enum {
		ARRAY_LENGTH =
		    (PAGE_L1_SIZE - sizeof info) / sizeof (allocatable_page)
	};
	allocatable_page page_array[ARRAY_LENGTH];

public:
	void* alloc(uptr size);
};

void* allocatable_page_array::alloc(uptr size)
{
	const int page_num = info.page_num;

	int pages = 0;
	for (int i = 0; i < ARRAY_LENGTH; ++i) {
		if (pages >= page_num)
			break;

		if (page_array[i].max_free_bytes == 0)
			continue;

		if (page_array[i].max_free_bytes >= size)
			;

		++pages;
	}

	return 0;
}


/// @brief カーネル自信が使うメモリを管理する。
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
	void* alloc_from_array(uptr size);

public:
	void* alloc(uptr size);
};

void* kernel_memory_::alloc_from_array(uptr size)
{
	for (allocatable_page_array* ary = allocatable_chain.get_head();
	    ary != 0;
	    ary = allocatable_chain.get_next())
	{
		ary->alloc_from_page(size);
	}

	return 0;
}

void* kernel_memory_::alloc(uptr size)
{
	return alloc_on_array(size);
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

