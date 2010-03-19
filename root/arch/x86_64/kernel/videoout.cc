// @file    arch/x86_64/kernel/setup/videoout.cc
// @author  Kato Takeshi
// @brief   Output only video destination.
//
// (C) 2009-2010 Kato Takeshi.

#include "output.hh"

#include "mem.hh"
#include "term.hh"


// @brief  Scroll.
// @param n  Scroll rows.
void VideoOutput::roll(int n)
{
	if (n < 0)
		return;

	if (n > height)
		n = height;

	memory_move(&vram[0], &vram[width * 2 * n],
		width * (height - n) * 2);

	char* space = &vram[width * (height - n) * 2];
	const int space_size = width * n * 2;
	for (int i = 0; i < space_size; i += 2) {
		space[i] = ' ';
		space[i + 1] = 0x00;
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
}


// @brief  Output 1 charcter.
// @param c  Character.
void VideoOutput::PutC(char c)
{
	if (c == '\n') {
		cur_col = 0;
		cur_row++;
	} else {
		const int cur = (width * cur_row + cur_col) * 2;
		vram[cur] = c;
		vram[cur + 1] = 0x0f;

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

