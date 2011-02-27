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

	void dump(kout& dump) {
		dump.c('[').u32hex(seed1)
		    .c(',').u32hex(seed2)
		    .c(',').u32hex(seed3).c(']');
	}
};

struct data {
	chain_link<data> link;
	uptr size;

	chain_link<data>& chain_hook() { return link; }
};

void memory_test()
{
	chain<data, &data::chain_hook> ch;
	test_rand rnd;

	for (;;) {
		arch::wait(0x400000);

		uptr total = 0;
		uptr total_max = 0;
		for (int i = 0; i < 100000; ++i) {
			uptr size =
			    rnd(0x200000 - 8 - sizeof (data)) + sizeof (data);

			data* p = (data*)memory::alloc(size);
//dump().udec(size)(':')(p)();
			if (p) {
				total += size;
				p->size = size;
				ch.insert_head(p);
			} else {
				if (total_max < total)
					total_max = total;
				p = ch.remove_head();
				total -= p->size;
//dump()('?')(p);
				memory::free(p);
//dump()('.');
			}
		}
rnd.dump(log());
log().u64hex(total_max).endl();

		for (;;) {
			data* p = ch.remove_head();
			if (p == 0)
				break;
			memory::free(p);
		}
	}
}

void test()
{
	memory_test();
}

