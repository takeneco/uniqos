/// @file   output.hh
/// @brief  Kernel debug message destination.
//
// (C) 2011 KATO Takeshi
//

#ifndef _ARCH_X86_64_KERNEL_OUTPUT_HH_
#define _ARCH_X86_64_KERNEL_OUTPUT_HH_

#include "log.hh"


/// @brief  Output only kernel video term.

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


/// @brief  Output only serial port term.

class serial_kout : public kout
{
	u16 base_port;

public:
	enum {
		COM1_BASEPORT = 0x03f8,
		COM2_BASEPORT = 0x02f8,

		COM1_PICIRQ = 4,
		COM2_PICIRQ = 3,

		OUT_BUF_SIZE = 14,
	};

	void init(u16 com_base_port);

	static void writec(kernel_log* self, u8 c);
};

serial_kout& serial_get_kout();
void serial_kout_init();


extern "C" void on_serial_intr_com1();
extern "C" void on_serial_intr_com2();


kern_output* kern_get_out();


#endif  // Include guard.

