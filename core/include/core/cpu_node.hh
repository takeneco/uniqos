/// @file  core/cpu_node.hh

//  Uniqos  --  Unique Operating System
//  (C) 2012-2015 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef CORE_CPU_NODE_HH_
#define CORE_CPU_NODE_HH_

#include <arch/pagetable.hh>
#include <config.h>
#include <core/basic.hh>
#include <core/message_queue.hh>
#include <core/thread_sched.hh>


class page_pool;

/// Architecture independent part of cpu control.
class cpu_node
{
	DISALLOW_COPY_AND_ASSIGN(cpu_node);

public:
	cpu_node(cpu_id cpunode_id);

	cause::t set_page_pool_cnt(int cnt);
	cause::t set_page_pool(int pri, page_pool* pp);

	cause::t setup();

	cpu_id get_cpu_node_id() const { return cpu_node_id; }

	void     attach_thread(thread* t);
	cause::t detach_thread(thread* t);
	void     ready_thread(thread* t);
	void     ready_thread_np(thread* t);

	void force_set_running_thread(thread* t);

	thread_sched& get_thread_ctl() { return threads; }

	cause::t page_alloc(arch::page::TYPE page_type, uptr* padr);
	cause::t page_dealloc(arch::page::TYPE page_type, uptr padr);

private:
	static void preempt_wait();

protected:
	cpu_id       cpu_node_id;
	thread_sched threads;
	cpu_id_t     page_pool_cnt;
	page_pool* page_pools[CONFIG_MAX_CPUS];
};

cpu_id_t get_cpu_node_count();
cpu_node* get_cpu_node();
cpu_node* get_cpu_node(cpu_id_t cpuid);

namespace arch {
void post_intr_message(message* msg);
void post_cpu_message(message* msg);
void post_cpu_message(message* msg, cpu_node* cpu);
}  // namespace arch

void post_message(message* msg); //TODO:OBSOLETED

void preempt_disable();
void preempt_enable();

class preempt_disable_section
{
	cpu_node* cn;
public:
	preempt_disable_section();
	~preempt_disable_section();
};

class preempt_enable_section
{
	cpu_node* cn;
public:
	preempt_enable_section();
	~preempt_enable_section();
};


#endif  // include guard

