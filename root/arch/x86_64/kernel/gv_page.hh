/// @file  gv_page.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_GV_PAGE_HH_
#define ARCH_X86_64_INCLUDE_GV_PAGE_HH_

#include "arch.hh"
#include "basic_types.hh"
#include "chain.hh"
#include "event_queue.hh"
#include "interrupt_control.hh"

#include "global_vars.hh"
#include "page_ctl.hh"
#include "mempool_ctl.hh"
#include "irq_ctl.hh"
#include "rcspec.hh"


/// @brief  Global variable page.
//
/// カーネルの初期化時にまとめてメモリを割り当てる。
class gv_page
{
	arch::page_ctl    page_ctl_obj;
	mempool_ctl       mempool_ctl_obj;
	arch::irq_ctl     irq_ctl_obj;
	intr_ctl          intr_ctl_obj;
	event_queue       event_ctl_obj;
	resource_spec     rc_spec_obj;

public:
	gv_page() {}
	bool init();
};

inline bool gv_page::init()
{
	if (sizeof *this > arch::page::PHYS_L1_SIZE) {
		return false;
	}

	using namespace global_vars;

	gv.gv_page_obj   = this;
	gv.page_ctl_obj    = &page_ctl_obj;
	gv.mempool_ctl_obj = &mempool_ctl_obj;
	gv.irq_ctl_obj     = &irq_ctl_obj;
	gv.intr_ctl_obj    = &intr_ctl_obj;
	gv.event_ctl_obj   = &event_ctl_obj;
	gv.rc_spec         = &rc_spec_obj;

	return true;
}


namespace arch {

namespace kmem {

class page_header;
class allocatable_page;

/// ページの先頭にページヘッダ(page_header)を置く。
/// ページをピース(piece)に切り出してメモリを割り当てる。
/// 割当済みピースの先頭には hold_piece_header を置く。
/// 空きピースの先頭には free_piece_header を置く。

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
	    piece_bytes(piece_bytes_)
	{
		page_offset = static_cast<u32>(
		    reinterpret_cast<uptr>(this) -
		    reinterpret_cast<uptr>(page_header_)
		);
	}
	u32 get_bytes() const {
		return piece_bytes;
	}
	void* get_memory_ptr() {
		return this + 1;
	}
	page_header* get_page_header() {
		const uptr p = reinterpret_cast<uptr>(this) - page_offset;
		return reinterpret_cast<page_header*>(p);
	}

	static hold_piece_header* ptr_to_hold_piece(void* p) {
		return reinterpret_cast<hold_piece_header*>(p) - 1;
	}
};


/// @brief 空きピースのヘッダ
class free_piece_header
{
public:
	/// ヘッダも含めたサイズ
	u32  piece_bytes;
	int  reserved; // for lock

	bichain_node<free_piece_header> chain_node_;

public:
	free_piece_header(u32 bytes) : piece_bytes(bytes) {}

	bichain_node<free_piece_header>& chain_hook() {
		return chain_node_;
	}

	u32 get_bytes() const {
		return piece_bytes;
	}
	/// 後ろにある piece のアドレスを返す。
	uptr get_after_piece_adr() const {
		return reinterpret_cast<uptr>(this) + piece_bytes;
	}

	/// 後ろにある hold_piece を結合する。
	void combine(hold_piece_header* after_piece) {
		piece_bytes += after_piece->get_bytes();
	}
	void combine(free_piece_header* after_piece) {
		piece_bytes += after_piece->piece_bytes;
	}

	/// @brief メモリを切り出す。
	//
	/// メモリを後ろから切り取って、切り取ったメモリのポインタを返す。
	void* cut(u32 bytes) {
		piece_bytes -= bytes;
		return reinterpret_cast<u8*>(this) + piece_bytes;
	}
};


/// @brief ページ内のメモリ割当状況を管理する。
class page_header
{
	u32  page_size;
	int  reserved;
	bibochain<free_piece_header, &free_piece_header::chain_hook>
	    free_chain;
	allocatable_page* status;

public:
	page_header(u32 page_size_, allocatable_page* status_);
	~page_header() {};

	/// ページ内に hold_piece があれば true を返す。
	bool is_holded() const {
		const free_piece_header* p = free_chain.head();
		return p->get_bytes() != page_size - sizeof *this;
	}

	free_piece_header* search_free_piece(uptr bytes);
	u32 search_max_free_bytes() const;

	void remove_free(free_piece_header* piece) {
		free_chain.remove(piece);
	}
	cause::stype set_free(hold_piece_header* piece);
};


/// @brief 割当可能な空きメモリを含むページを管理する。
//
/// 空きメモリを検索するときに、空きメモリのサイズの部分が
/// キャッシュに乗るようにする。
class allocatable_page
{
	/// ページが含む最も大きい空きメモリサイズ。
	u32  max_free_bytes;
	/// allocatable_page_array からのオフセット
	u32  array_offset;
	/// ページの先頭アドレス。
	uptr page_adr;

	enum { NOT_CAPTURED = 0xffffffff };

public:
	allocatable_page() : max_free_bytes(NOT_CAPTURED) {}

	void init(void* array) {
		const u8* const a = reinterpret_cast<const u8*>(array);
		const u8* const b = reinterpret_cast<const u8*>(this);
		array_offset = b - a;
	}

	bool is_captured() const {
		return max_free_bytes != NOT_CAPTURED;
	}
	u32 get_max_free_bytes() const {
		return max_free_bytes;
	}
	page_header* get_page_header() {
		return reinterpret_cast<page_header*>(page_adr);
	}
	/// free_piece の数が増えたり、
	/// free_piece のサイズが大きくなったときに呼ばれる。
	void growed_free_piece(u32 bytes) {
		if (max_free_bytes < bytes)
			max_free_bytes = bytes;
	}

	void init(uptr page_adr_, u32 page_size);

	hold_piece_header* alloc(uptr bytes);
};


/// @brief allocatable_page の配列。
//
/// チェインでつなぐ。
class allocatable_page_array
{
	// sizeof info と書けるようにまとめた。
	struct array_info
	{
		bichain_node<allocatable_page_array> chain_node_;
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
		    (arch::page::PHYS_L1_SIZE - sizeof (array_info)) /
		    sizeof (allocatable_page)
	};
	allocatable_page page_array[ARRAY_LENGTH];

private:
	allocatable_page* new_entry();

public:
	allocatable_page_array();

	bichain_node<allocatable_page_array>& chain_hook() {
		return info.chain_node_;
	}

	allocatable_page* add_page(uptr page_adr, u32 page_size);
	hold_piece_header* alloc(uptr bytes);
};

inline allocatable_page_array::allocatable_page_array()
	: info()
{
	for (int i = 0; i < ARRAY_LENGTH; ++i)
		page_array[i].init(this);
}
/*
/// @brief カーネル自身が使うメモリを管理する。
//
/// @test このクラスは物理メモリ１ページに収まらなければならない。
/// (初期化処理で物理ページを割り当てているだけなので)
class kernel_memory
{
	typedef bibochain
	<allocatable_page_array, &allocatable_page_array::chain_hook>
	    allocatable_chain_type;

	allocatable_chain_type allocatable_chain;

private:
	hold_piece_header* alloc_from_existpage(uptr size);
	hold_piece_header* alloc_from_newpage(uptr size);

public:
	void* alloc(uptr size);
	cause::stype free(void* ptr);
};
*/
}  // namespace kmem

}  // namespace arch


#endif  // include guard

