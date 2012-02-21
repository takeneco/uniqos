/// @file  test.cc
//
// (C) 2010 KATO Takeshi
//

#include "basic_types.hh"
#include "global_vars.hh"
#include "mempool.hh"
#include "page_ctl.hh"
#include "log.hh"

#include "memory_allocate.hh"
#include <processor.hh>


void event_drive();

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
	void init(u32 s1=0, u32 s2=0) {
		seed1 = s1;
		seed2 = s2;
		seed3 = 0;
	}

	operator u32 () { return get(); }
	operator u64 () {
		const u64 tmp = (u64)get() << 32;
		return tmp | get();
	}
	u32 operator () () { return get(); }
	u32 operator () (u32 top) { return get() % top; }

	void dump(log_target& dump) {
		dump.c('[').u(seed1, 16)
		    .c(',').u(seed2, 16)
		    .c(',').u(seed3, 16).c(']');
	}
};

test_rand rnd;

struct data {
	chain_node<data> link;
	uptr size;

	chain_node<data>& chain_hook() { return link; }
};

void memory_test()
{
	chain<data, &data::chain_hook> ch;
	test_rand rnd;

	for (;;) {
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
log lg;
rnd.dump(lg);
log().u(total_max, 16).endl();

		for (;;) {
			data* p = ch.remove_head();
			if (p == 0)
				break;
			memory::free(p);
		}
	}
}

void mempool_test()
{
	log lg;

	static u64 test_number = 0;
	log().u(test_number, 16)("|seed:");
	rnd.dump(lg);
	log();
	++test_number;

	mempool* mp = mempool_create_shared(100);

	const int N = 15;
	chain<data, &data::chain_hook> ch[N];

	int n;
	int cnts[N] = {0};
	for (n = 0; n < 0x4000; ++n) {
		data* p = (data*)mp->alloc();
		if (!p)
			break;

		for (int i = 0; i < N; ++i) {
			data* q = ch[i].head();
			for (; q; q = ch[i].next(q)) {
				if (p == q) {
					log()("XXX n=").u(n,16)(" / i=").u(i)();
					mp->dump(lg);
					for (;;) native::hlt();
				}
			}
		}

		u32 idx = rnd(N);

		ch[idx].insert_head(p);
		++cnts[idx];
	}

	log()("sum n=").u(n,16)();
	global_vars::gv.page_ctl_obj->dump(lg);

	for (int i = 0; i < N; ++i) {
		int cnt;
		for (cnt = 0;; ++cnt) {
			data* d = ch[i].remove_head();
			if (!d)
				break;
			mp->free(d);
		}

		mp->collect_free_pages();
	}

	log()("after collect_free_pages()")();
	global_vars::gv.page_ctl_obj->dump(lg);
}

void switch_test()
{
	thread_ctl& tc =
	    global_vars::gv.logical_cpu_obj_array[0].get_thread_ctl();

	for (int i = 0;; ++i) {
		mempool_test();
		//log()(".");
		//if ((i & 63) == 63)
		//	log()();
		//tc.sleep_running_thread();
	}
}

bool test_init()
{
	rnd.init(0, 0);
	return true;
}

void test(void*)
{
	//memory_test();
	//mempool_test();
	switch_test();
}

