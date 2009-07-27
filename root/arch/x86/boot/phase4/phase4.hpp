/**
 * @file    arch/x86/boot/phase4/phase4.hpp
 * @version 0.0.4
 * @date    2009-07-28
 * @author  Kato.T
 * @brief   phase4 で使用する機能の宣言。
 */
// (C) Kato.T 2009

#ifndef _ARCH_X86_BOOT_PHASE4_PHASE4_HPP
#define _ARCH_X86_BOOT_PHASE4_PHASE4_HPP

#include <cstddef>

#include "btypes.hpp"
#include "boot.h"


// メモリ管理
struct memmap_entry;
struct memmgr
{
	// 空き領域リスト
	memmap_entry* free_list;

	// 割り当て済み領域リスト
	memmap_entry* nofree_list;
};

void  memmgr_init(memmgr* mm);
void* memmgr_alloc(memmgr* mm, size_t size);
void  memmgr_free(memmgr* mm, void* p);
void* operator new(std::size_t s, memmgr*);
void* operator new[](std::size_t s, memmgr*);
void  operator delete(void* p, memmgr*);
void  operator delete[](void* p, memmgr*);


/**
 * 出力専用端末の基本クラス。
 */
class outterm
{
public:
	outterm* next;

public:

	virtual void putc(char ch) = 0;
};

/**
 * 複数の outterm へテキストを同時出力する。
 */
class term_chain
{
	outterm* terms;
public:
	void init();
	void add_term(outterm* term);
	term_chain* putc(char ch);
	term_chain* puts(const char* str);
	term_chain* putu32(_u32 n);
	term_chain* putu8x(_u8 n);
	term_chain* putu16x(_u16 n);
	term_chain* putu32x(_u32 n);
	term_chain* putu64x(_u64 n);
};

/**
 * 出力用シリアル端末。
 */
class com_term : public outterm
{
	_u16 base_port;
	_u16 irq;
	int out_buf_size;
	int out_buf_left;

	/* static */ int next_data(int ptr);
	void tx_buf();
public:
	void init(_u16 dev_port, _u16 pic_irq);
	virtual void putc(char ch);
	void on_interrupt();

public:
	enum {
		COM1_BASE = 0x03f8,
		COM2_BASE = 0x02f8,

		COM1_IRQ = 4,
		COM2_IRQ = 3,
	};
};


/**
 * テキストモード画面出力
 */
class video_term : public outterm
{
	int   width;
	int   height;
	char* vram;
	int   cur_row;
	int   cur_col;

	void roll(unsigned int n);
public:
	void init(int w, int h, _u32 vram_addr);
	void set_cur(int row, int col) { cur_row = row; cur_col = col; }
	virtual void putc(char ch);
	video_term* puts(const char* str);
	video_term* putu32(_u32 n);
	video_term* putu32x(_u32 n);
	video_term* putu64x(_u64 n);
};

inline void set_video_term(video_term* p) {
	*reinterpret_cast<video_term**>(PH4_VIDEOTERM) = p;
}
inline video_term* get_video_term() {
	return *reinterpret_cast<video_term**>(PH4_VIDEOTERM);
}

void* memcpy(void* dest, const void* src, std::size_t n);

// lzma wrapper

bool lzma_decode(
	memmgr*     mm,
	_u8*        src,
	std::size_t src_len,
	_u8*        dest,
	std::size_t dest_len);


#endif  // _ARCH_X86_BOOT_PHASE4_PHASE4_HPP

