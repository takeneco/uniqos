/// @file   cpu_ctl.cc
//
// (C) 2010-2012 KATO Takeshi
//

#include "arch.hh"
#include "global_vars.hh"
#include "kerninit.hh"
#include "log.hh"
#include "mempool.hh"
#include "mpspec.hh"
#include <native_ops.hh>
#include "placement_new.hh"
#include <processor.hh>
#include "string.hh"


namespace {

enum {
	IST_BYTES = 0x2000,  // size of ist
};

struct ist_footer_layout
{
	arch::regset** regs;
	processor* proc;
};

}  // namespace

namespace arch {

processor* get_current_cpu()
{
	return &global_vars::gv.logical_cpu_obj_array[0];
}

}  // namespace arch

cause::stype cpu_init()
{
	cpu_share* cpu_share_obj =
	    new (mem_alloc(sizeof (cpu_share))) cpu_share;
	global_vars::gv.cpu_share_obj = cpu_share_obj;

	cause::stype r = cpu_share_obj->init();
	if (is_fail(r))
		return r;

	arch::cpu_ctl::IDT* idt =
	    new (mem_alloc(sizeof (arch::cpu_ctl::IDT))) arch::cpu_ctl::IDT;

	//intr_init();
	idt->init();

	processor* logi_cpus =
	    new (mem_alloc(sizeof (processor[1]))) processor[1];
	global_vars::gv.logical_cpu_obj_array = logi_cpus;

	r = logi_cpus[0].init();
	if (is_fail(r))
		return r;

	r = logi_cpus[0].load();
	if (is_fail(r))
		return r;

	//pic_init();

	return cause::OK;
}


// cpu_share


cpu_share::cpu_share()
{
}

cause::stype cpu_share::init()
{
	cause::stype r = mps.load();
	if (is_fail(r))
		return r;

	return cause::OK;
}


// arch::cpu_ctl

namespace arch {

cause::stype cpu_ctl::init()
{
	gdt.null_entry.set_null();
	gdt.kern_code.set(0);
	gdt.kern_data.set(0);
	gdt.user_code.set(3);
	gdt.user_data.set(3);
	gdt.tss.set(&tss, sizeof tss, 0);

	//mem_fill(sizeof tss, 0, &tss);
	tss.iomap_base = sizeof tss;

	mempool* ist_mp = mempool_create_shared(IST_BYTES);
	void* ist_intr = ist_mp->alloc();
	void* ist_trap = ist_mp->alloc();
	mempool_release_shared(ist_mp);
	if (!ist_intr || !ist_trap)
		return cause::NOMEM;

	tss.set_ist(write_ist_layout(ist_intr), IST_INTR);
	tss.set_ist(write_ist_layout(ist_trap), IST_TRAP);

	return cause::OK;
}

cause::stype cpu_ctl::load()
{
	native::gdt_ptr64 gdtptr;
	gdtptr.set(sizeof gdt, &gdt);
	native::lgdt(&gdtptr);

	native::set_ds(GDT::kern_data_offset());
	native::set_es(GDT::kern_data_offset());
	native::set_fs(GDT::kern_data_offset());
	native::set_gs(GDT::kern_data_offset());
	native::set_ss(GDT::kern_data_offset());

	native::ltr(GDT::tss_offset());

	return cause::OK;
}

void cpu_ctl::set_running_thread(thread* t)
{
	running_thread_regset = t->ref_regset();
}

/// @brief IST へ ist_footer_layout を書く。
/// @return IST として使用するポインタを返す。
//
/// 割り込みハンドラが IST から情報をたどれるようにする。
void* cpu_ctl::write_ist_layout(void* mem)
{
	ist_footer_layout* istf = static_cast<ist_footer_layout*>(mem);

	istf -= 1;

	istf->proc = static_cast<processor*>(this);
	istf->regs = &this->running_thread_regset;

	return istf;
}


void intr_enable()
{
	native::cli();
}

void intr_disable()
{
	native::sti();
}

void halt() {
	native::hlt();
}

}  // namespace arch

