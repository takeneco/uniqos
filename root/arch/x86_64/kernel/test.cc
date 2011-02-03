/// @file  kernel_memory.cc
/// @brief Kernel internal virtual memory management.
//
// (C) 2010 KATO Takeshi
//

#include "btypes.hh"
#include "chain.hh"
#include "kerninit.hh"
#include "memory_allocate.hh"
#include "output.hh"


class test_rand
{
	u32 seed1, seed2, seed3;

	u32 get() {
		seed3 = seed2 + 1;
		seed2 = seed1;
		return seed1 = seed2 + seed3;
	}
public:
	test_rand(u32 s1=0, u32 s2=0) : seed1(s1), seed2(s2), seed3(0) {}

	operator u32 () { return get(); }
	operator u64 () {
		const u64 tmp = (u64)get() << 32;
		return tmp | get();
	}
	u32 operator () () { return get(); }
	u32 operator () (u32 top) { return get() % top; }
};

struct data {
	chain_link<data> link;

	chain_link<data>& chain_hook() { return link; }
};

void memory_test()
{
	chain<data, &data::chain_hook> ch;

	uptr total = 0;

	for (int i = 0; ; ++i) {
		ko().udec(i).c(':');

		arch::wait(0x400000);

		ko().c('a');
		uptr s = 0;
		for (;;) {
			if (i == 5 && s >= 0x172208) {
				ko_set(1, true);
				ko().c('x');
			}
			data* p = (data*)memory::alloc(sizeof (data));
			if (p) {
				s += sizeof (data);
				ch.insert_head(p);
			} else {
				break;
			}
		}

		ko().c('b');
		if (total == 0) {
			total = s;
			ko().	str("total = ").
				u64hex(total).
				endl();
		}
		else if (total != s) {
			ko().	str("s = ").
				u64hex(s).
				endl();
		}

		ko().c('c');
		for (;;) {
			data* p = ch.get_head();
			if (p == 0)
				break;
			ch.remove_head();
			memory::free(p);
		}
		ko().c('d').endl();
	}

	ko().c('?').endl();
}

void test()
{
	memory_test();
}

