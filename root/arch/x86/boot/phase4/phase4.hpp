/**
 * @file    arch/x86/boot/phase4/phase4.hpp
 * @version 0.0.3
 * @date    2009-07-22
 * @author  Kato.T
 *
 * phase4 で使用する機能の宣言。
 */
// (C) Kato.T 2009

#ifndef _ARCH_X86_BOOT_PHASE4_PHASE4_HPP
#define _ARCH_X86_BOOT_PHASE4_PHASE4_HPP

#include <cstddef>
#include "btypes.hpp"


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

	/// 出力バッファ
	char data_buf[256];

	// buf 中の未出力データの先頭と終端
	int data_head;
	int data_tail;

	// 出力先バッファのサイズ
	int out_buf_size;

	// 出力先バッファの空き
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


void* memcpy(void* dest, const void* src, std::size_t n);

void  memmgr_init();
void* operator new(std::size_t s);
void  operator delete(void* p);


#endif  // _ARCH_X86_BOOT_PHASE4_PHASE4_HPP

