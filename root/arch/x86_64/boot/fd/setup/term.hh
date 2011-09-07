// @file    term.hh
// @brief   カーネルセットアップ中のメッセージ出力先。
//
// (C) 2010 Kato Takeshi

#ifndef ARCH_X86_64_KERNEL_SETUP_TERM_HH_
#define ARCH_X86_64_KERNEL_SETUP_TERM_HH_

#include "log.hh"


/// @brief  output setup log to memory.
class memory_log
{
public:

};

/// @brief  Output terminal base class.

class outterm
{
public:
	outterm* next;
	virtual void putc(char c) = 0;
};


/// @brief  複数の outterm へテキストを同時出力する。

class term_chain
{
	outterm* terms;
public:
	void init();
	void add_term(outterm* term);
	term_chain* putc(char c);
	term_chain* puts(const char* str);
	term_chain* putu64(u64 n);
	term_chain* putu8x(u8 n);
	term_chain* putu16x(u16 n);
	term_chain* putu32x(u32 n);
	term_chain* putu64x(u64 n);
};


/// @brief  Output only kernel serial term.

class com_term : public outterm
{
	u16 base_port;
	u16 irq;
	int out_buf_size;
	int out_buf_left;

	/* static */ int next_data(int ptr);
	void tx_buf();
public:
	void init(u16 dev_port, u16 pic_irq);
	virtual void putc(char c);
	void on_interrupt();

public:
	enum {
		COM1_BASE = 0x03f8,
		COM2_BASE = 0x02f8,

		COM1_IRQ = 4,
		COM2_IRQ = 3,
	};
};


/// @brief  Output only kernel video term.

class video_term : public outterm
{
	int   width;
	int   height;
	char* vram;
	int   cur_row;
	int   cur_col;

	void roll(int n);
public:
	void init(int w, int h, u64 vram_addr);
	void set_cur(int row, int col) { cur_row = row; cur_col = col; }
	void get_cur(int* row, int* col) { *row = cur_row; *col = cur_col; }
	virtual void putc(char c);
};


#endif  // Include guard

