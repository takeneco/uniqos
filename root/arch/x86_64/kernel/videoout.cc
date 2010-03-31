// @file    arch/x86_64/kernel/videoout.cc
// @author  Kato Takeshi
// @brief   Output only video destination.
//
// (C) 2009-2010 Kato Takeshi.

#include "output.hh"

#include "string.hh"


// @brief  Scroll.
// @param n  Scroll rows.
void VideoOutput::roll(int n)
{
	if (n < 0)
		return;

	if (n > height)
		n = height;

	for (int row = 1; row <= n; row++) {
		int clone_row = clone_cur_row + row;
		if (clone_row >= height)
			clone_row -= height;
		clone_row *= width;
		for (int col = 0; col < width; col++) {
			vram_clone[(clone_row + col) * 2] = ' ';
			vram_clone[(clone_row + col) * 2 + 1] = 0;
		}
	}
	clone_cur_row += n;

	for (int row = 0; row < height; row++) {
		int vram_col = width * row * 2;
		int clone_row = row + clone_cur_row + 1;
		if (clone_row >= height)
			clone_row -= height;
		int clone_col = clone_row * width * 2;
		for (int col = 0; col < width; col++) {
			vram[vram_col++] = vram_clone[clone_col++];
			vram[vram_col++] = vram_clone[clone_col++];
		}
	}

	while (clone_cur_row >= height)
		clone_cur_row -= height;

//	char* dest = &vram[0];
//	char* src = &vram[width * 2 * n];
/*
	MemoryMove(width * (height - n) * 2,
	    &vram[width * 2 * n], &vram[0]);
	//memory_move(&vram[0], &vram[width * 2 * n],
	//	width * (height - n) * 2);
*/
/*
	char* const vram_space = &vram[width * (height - n) * 2];
	const int space_size = width * n * 2;
	for (int i = 0; i < space_size; i += 2) {
		vram_space[i] = ' ';
		vram_space[i + 1] = 0x00;
	}
*/
}

// @brief  Output 1 charcter.
//
void VideoOutput::put(char c)
{
	if (c == '\n') {
		cur_col = 0;
		cur_row++;
	} else {
		const int cur = (width * cur_row + cur_col) * 2;
		vram[cur] = c;
		vram[cur + 1] = 0x0f;

		const int clone_cur = (width * clone_cur_row + cur_col) * 2;
		vram_clone[clone_cur] = c;
		vram_clone[clone_cur + 1] = 0x0f;

		if (++cur_col == width) {
			cur_col = 0;
			cur_row++;
		}
	}

	if (cur_row == height) {
		roll(1);
		cur_row--;
	}
}


// @brief  Initialize.
// @param w         Console width chars.
// @param h         Console height chars.
// @param vram_addr VRAM head address.
void VideoOutput::Init(int w, int h, u64 vram_addr)
{
	width = w;
	height = h;
	vram = reinterpret_cast<char*>(vram_addr);

	cur_row = cur_col = 0;
	clone_cur_row = 0;
}


int VideoOutput::Write(
    IOVector* Vectors,
    int       VectorCount,
    ucpu      Offset)
{
	Offset = Offset;

	for (int i1 = 0; i1 < VectorCount; i1++) {
		const ucpu n = Vectors[i1].Bytes;
		const char* addr =
		    reinterpret_cast<const char*>(Vectors[i1].Address);
		for (ucpu i2 = 0; i2 < n; i2++) {
			put(addr[i2]);
		}
	}

	return cause::OK;
}

