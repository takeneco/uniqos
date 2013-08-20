/// @file  test.cc

#include <cpu_node.hh>
#include <global_vars.hh>
#include <log.hh>
#include <mempool_ctl.hh>
#include <native_ops.hh>
#include <new_ops.hh>
#include <page_pool.hh>
#include <spinlock.hh>
#include <timer_ctl.hh>


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
	chain_node<data> link;
	uptr size;

	chain_node<data>& chain_hook() { return link; }
};

typedef timer_message_with<thread*> wakeup;
void wakeup_handler(message* msg)
{
	wakeup* w = (wakeup*)msg;

	//w->data->ready();

	thread_queue& tc = get_cpu_node()->get_thread_ctl();
	//tc.ready_np(w->data);
	tc.ready(w->data);

	w->data = 0;
}

void mempool_test()
{
	obj_edge oe1("obj1");
	obj_node on1("type1", oe1);
	obj_edge oe2("obj2", on1);
	obj_node on2("type2", oe2);
	obj_edge oe3("obj3", on2);
	obj_node on3("type2", oe3);

	thread_queue& tc = get_cpu_node()->get_thread_ctl();
	thread* th = tc.get_running_thread();

	wakeup wakeupme;
	wakeupme.handler = wakeup_handler;
	//wakeupme.data = th;
	wakeupme.data = 0;
	wakeupme.nanosec_delay = 1000000000;

	log lg;
	on3.dump(lg);
	lg();

	test_number_lock->lock();
	const u64 tn = test_number;
	++test_number;
	test_number_lock->unlock();

	mempool* mp[2];
	cause::type r = mempool_acquire_shared(20, &mp[0]);
	if (is_fail(r))
		log()("!!! error r=").u(r)();

	r = mempool_acquire_shared(100, &mp[1]);
	if (is_fail(r))
		log()("!!! error r=").u(r)();

	preempt_disable();

	lg.p(th)("|T0:").u(tn)("|seed:");
	rnd.dump(lg);
	lg();

	for (int i = 0; i < global_vars::core.page_pool_cnt; ++i) {
		global_vars::core.page_pool_objs[i]->dump(lg, 1);
	}

	global_vars::core.mempool_ctl_obj->dump(lg);

	mp[0]->dump(lg, 1);
	mp[1]->dump(lg, 1);

	preempt_enable();

	const int N = 15;
	chain<data, &data::chain_hook> ch[2][N];

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
			data* q = ch[mpi][i].head();
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

		ch[mpi][idx].insert_head(p);
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
				data* d = ch[mpi][i].remove_head();
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

	mempool_release_shared(mp[0]);
	mempool_release_shared(mp[1]);

	log().p(th)("|TEST:").u(tn)("|sum:").x(n)();
}

bool test_init()
{
	rnd.init(0, 0);
	test_number = 0;
	test_number_lock = new (mem_alloc(sizeof (spin_lock))) spin_lock;
	return true;
}

void test(void*)
{
	for (;;) {
		mempool_test();
	}
}

