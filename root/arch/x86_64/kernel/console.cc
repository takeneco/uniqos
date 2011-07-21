/// @file  console.cc
//
// (C) 2011 KATO Takeshi
//

#include "arch.hh"
#include "fileif.hh"
#include "memory_allocate.hh"
#include "placement_new.hh"

#include "output.hh"


namespace {

class console_ctrl : public file_interface
{
	int width;
	int height;
	char* vram;

	int cur_row;
	int cur_col;

	char* vram_clone;
	int clone_cur_row;

public:
	/// @todo: do not use global var.
	static file_ops console_ops;

public:
	console_ctrl(int width_, int height_, u64 vram_adr_);
	cause::stype configure();

private:
	void roll(int n);
	void put(char c);
	cause::stype write_(const char* data, uptr size);

public:
	static cause::stype write(
	    file_interface* self, const void* data, uptr size, uptr offset);
	void dump();
};

file_ops console_ctrl::console_ops;

console_ctrl::console_ctrl(int width_, int height_, u64 vram_adr_) :
	width(width_),
	height(height_),
	vram(reinterpret_cast<char*>(arch::pmem::direct_map(vram_adr_))),
	cur_row(0),
	cur_col(0),
	clone_cur_row(0)
{
	ops = &console_ops;
}

cause::stype console_ctrl::configure()
{
	vram_clone = reinterpret_cast<char*>(memory::alloc(width * height * 2));

	if (vram_clone == 0)
		return cause::NO_MEMORY;

	return cause::OK;
}

/// @brief scroll.
/// @param n  scroll rows.
void console_ctrl::roll(int n)
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
}

void console_ctrl::put(char c)
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

// TODO: exclusive
cause::stype console_ctrl::write_(const char* data, uptr size)
{
	for (uptr i = 0; i < size; ++i)
		put(data[i]);

	return cause::OK;
}

cause::stype console_ctrl::write(
    file_interface* self, const void* data, uptr size, uptr offset)
{
	offset = offset;

	console_ctrl* console = reinterpret_cast<console_ctrl*>(self);

	return console->write_(reinterpret_cast<const char*>(data), size);
}

}

file_interface* attach_console(int w, int h, u64 vram_adr)
{
	///////
	console_ctrl::console_ops.write = console_ctrl::write;
	///////

	void* mem = memory::alloc(sizeof (console_ctrl));
	console_ctrl* console = new (mem) console_ctrl(w, h, vram_adr);

	console->configure();

	return console;
}


