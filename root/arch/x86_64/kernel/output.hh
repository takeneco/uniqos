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

class VideoOutput : public KernOutput
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

	virtual int Write(
	    IOVector* Vectors,
	    int       VectorCount,
	    ucpu      Offset);
};


#endif  // Include guard.

