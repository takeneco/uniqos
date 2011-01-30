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


struct data {
	chain_link<data> link;

	chain_link<data>& chain_hook() { return link; }
};

void memory_test()
{
	kern_output* out = kern_get_out();

	chain<data, &data::chain_hook> ch;

	uptr total = 0;

	for (int i = 0; ; ++i) {
		out->put_udec(i)->put_c(':');

		arch::wait(0x400000);

		out->put_c('a');
		uptr s = 0;
		for (;;) {
			if (i == 5 && s >= 0x172200) {
				out->put_c('x');
			}
			data* p = (data*)memory::alloc(sizeof (data));
			if (p) {
				s += sizeof (data);
				ch.insert_head(p);
			} else {
				break;
			}
		}
		out->put_c('b');
		if (total == 0) {
			total = s;
			out->	put_str("total = ")->
				put_u64hex(total)->
				put_endl();
		}
		else if (total != s) {
			out->	put_str("s = ")->
				put_u64hex(s)->
				put_endl();
		}

		out->put_c('c');
		for (;;) {
			data* p = ch.get_head();
			if (p == 0)
				break;
			ch.remove_head();
			memory::free(p);
		}
		out->put_c('d')->put_endl();
	}

	out->put_c('?')->put_endl();
}

void test()
{
	memory_test();
}

