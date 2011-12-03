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

kern_output* kern_get_out();


#endif  // Include guard.

