/// @file  test.cc

#include <core/cpu_node.hh>
#include <global_vars.hh>
#include <core/log.hh>
#include <core/mempool.hh>
#include <core/new_ops.hh>
#include <core/page_pool.hh>
#include <core/timer_ctl.hh>
#include <x86/native_ops.hh>


void event_drive();

class test_rand
{
	volatile u32 seed1, seed2, seed3;
	spin_lock lock;

	u32 get() {
		seed3 = seed2 + 1;
		seed2 = seed1;
		return seed1 = seed2 + seed3;
	}

public:
	test_rand(u32 s1=0, u32 s2=0) :
		seed1(s1),
		seed2(s2),
		seed3(0)
	{}
	void init(u32 s1=0, u32 s2=0) {
		seed1 = s1;
		seed2 = s2;
		seed3 = 0;
	}

	u32 get_u32() {
		spin_lock_section _sls(lock);
		return get();
	}
	u64 get_u64() {
		spin_lock_section _sls(lock);
		const u64 tmp = (u64)get() << 32;
		return tmp | get();
	}

	operator u32 () { return get_u32(); }
	operator u64 () { return get_u64(); }
	u32 operator () () { return get_u32(); }
	u32 operator () (u32 top) { return get_u32() % top; }

	void read_seed(u32* s1, u32* s2, u32* s3) {
		spin_lock_section _sls(lock);
		*s1 = seed1;
		*s2 = seed2;
		*s3 = seed3;
	}
	void dump(output_buffer& dump) {
		spin_lock_section _sls(lock);
		dump.c('[').x(seed1).c(',').x(seed2).c(',').x(seed3).c(']');
	}
};

namespace {

test_rand  rnd;
u64        test_number;
spin_lock* test_number_lock;

}  // namespace

struct data {
	forward_chain_node<data> link;
	uptr size;
};

typedef timer_message_with<thread*> wakeup;
void wakeup_handler(message* msg)
{
	wakeup* w = (wakeup*)msg;

	//w->data->ready();

	thread_sched& ts = get_cpu_node()->get_thread_ctl();
	//tc.ready_np(w->data);
	ts.ready(w->data);

	w->data = 0;
}

void mempool_test()
{
	thread* th = get_current_thread();

	wakeup wakeupme;
	wakeupme.handler = wakeup_handler;
	//wakeupme.data = th;
	wakeupme.data = 0;
	wakeupme.nanosec_delay = 1000000000;

	test_number_lock->lock();
	const u64 tn = test_number;
	++test_number;
	test_number_lock->unlock();

	mempool* mp[2];
	auto rmp = mempool::acquire_shared(20);
	if (is_fail(rmp))
		log()("!!! error r=").u(rmp.cause())();
	mp[0] = rmp;

	rmp = mempool::acquire_shared(100);
	if (is_fail(rmp))
		log()("!!! error r=").u(rmp.cause())();
	mp[1] = rmp;

	preempt_disable();

	log lg;

	lg.p(th)("|T0:").u(tn)("|seed:");
	rnd.dump(lg);
	lg();

	for (uint i = 0; i < global_vars::core.page_pool_nr; ++i) {
		global_vars::core.page_pool_objs[i]->dump(lg, 1);
	}

	//global_vars::core.mempool_ctl_obj->dump(lg);

	mp[0]->dump(lg, 1);
	mp[1]->dump(lg, 1);

	preempt_enable();

	const int N = 15;
	front_forward_chain<data, &data::link> ch[2][N];

	int n;
	int cnts[2][N] = {{0}};
	for (n = 0; n < 0x80000; ++n) {
		if (0==(n&0xff))
			log().p(th)("|n=").x(n)();
		int mpi = rnd(2);
		data* p = (data*)mp[mpi]->alloc();
		if (!p)
			break;

		for (int i = 0; i < N; ++i) {
			data* q = ch[mpi][i].front();
			for (; q; q = ch[mpi][i].next(q)) {
				if (p == q) {
					log()("!!! n=").x(n)(" / i=").u(i)();
					mp[mpi]->dump(lg, 3);
					lg();
					for (;;) native::hlt();
				}
			}
		}

		u32 idx = rnd(N);

		ch[mpi][idx].push_front(p);
		++cnts[mpi][idx];

		if (0==(n&0xff)&&!wakeupme.data) {
			wakeupme.handler = wakeup_handler;
			wakeupme.data = th;
			wakeupme.nanosec_delay = 1000000000;
			global_vars::core.timer_ctl_obj->set_timer(&wakeupme);
			sleep_current_thread();
		}
	}

	for (int i = 0; i < 2; ++i) {
		lg.p(th)("|T1:").u(tn)("|cnts[").u(i)("]:");
		for(int j = 0; j < N; ++j)
			lg.u(cnts[i][j])(",");
		lg();
	}

	for (int mpi = 0; mpi < 2; ++mpi) {
		for (int i = 0; i < N; ++i) {
			int cnt;
			for (cnt = 0;; ++cnt) {
				data* d = ch[mpi][i].pop_front();
				if (!d)
					break;
				mp[mpi]->dealloc(d);

				--cnts[mpi][i];
			}

			mp[mpi]->collect_free_pages();
		}
	}

	for (int i = 0; i < 2; ++i) {
		lg.p(th)("|T2:").u(tn)("|cnts[").u(i)("]:");
		for(int j = 0; j < N; ++j)
			lg.u(cnts[i][j])(",");
		lg();
	}

	mempool::release_shared(mp[0]);
	mempool::release_shared(mp[1]);

	log().p(th)("|TEST:").u(tn)("|sum:").x(n)();
}

bool test_init()
{
	rnd.init(0, 0);
	test_number = 0;
	test_number_lock = new (generic_mem()) spin_lock;
	return true;
}

void test(void*)
{
	for (;;) {
		mempool_test();
	}
}

