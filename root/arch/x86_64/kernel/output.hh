// @file   arch/x86_64/kernel/output.hh
// @author Kato Takeshi
// @brief  Kernel message destination.
//
// (C) 2010 Kato Takeshi.

#ifndef _ARCH_X86_64_KERNEL_OUTPUT_HH_
#define _ARCH_X86_64_KERNEL_OUTPUT_HH_

#include <cstddef>

#include "kout.hh"


// @brief  Output only kernel video term.

class VideoOutput : public kern_output
{
	int   width;
	int   height;
	char* vram;
	int   cur_row;
	int   cur_col;

	char  vram_clone[80 * 25 * 2];
	int   clone_cur_row;

	void roll(int n);
	void put(char c);
public:
	void Init(int w, int h, u64 vram_addr);
	void SetCur(int row, int col) { cur_row = row; cur_col = col; }
	void GetCur(int* row, int* col) { *row = cur_row; *col = cur_col; }

	virtual int write(
	    const io_vector* vectors,
	    int              vector_count,
	    ucpu             offset);
};


extern "C" void on_serial_intr_com1();
extern "C" void on_serial_intr_com2();

// @brief  Output only serial port term.

class serial_output : public kern_output
{
	friend void on_serial_intr_com1();
	friend void on_serial_intr_com2();

	u16 base_port;
//	u16 pic_irq;
	u16 out_buf_left;
public:
	enum {
		COM1_BASEPORT = 0x03f8,
		COM2_BASEPORT = 0x02f8,

		COM1_PICIRQ = 4,
		COM2_PICIRQ = 3,

		OUT_BUF_SIZE = 2,
	};

	void* operator new(std::size_t size, serial_output* obj) {
		size = size;
		return obj;
	}

	void init(u16 com_base_port, u16 com_pic_irq);

	virtual int write(
	    const io_vector* vectors,
	    int              vector_count,
	    ucpu             offset);
};

kern_output* kern_get_out();
serial_output* serial_get_out(int i);
void serial_output_init();


#endif  // Include guard.

