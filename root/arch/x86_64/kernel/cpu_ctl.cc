/// @file   cpu_ctl.cc
//
// (C) 2010-2011 KATO Takeshi
//

#include "cpu_ctl.hh"

#include "arch.hh"
#include "global_vars.hh"
#include "kerninit.hh"
#include "log.hh"
#include "mempool.hh"
#include "mpspec.hh"
#include "native_ops.hh"
#include "placement_new.hh"
#include "string.hh"


cause::stype cpu_init()
{
	cpu_ctl* cpu_ctl_obj = new (mem_alloc(sizeof (cpu_ctl))) cpu_ctl;
	global_vars::gv.cpu_ctl_obj = cpu_ctl_obj;

	cause::stype r = cpu_ctl_obj->init();
	if (is_fail(r))
		return r;

	r = cpu_ctl_obj->load();
	if (is_fail(r))
		return r;

	intr_init();

	//pic_init();

	return cause::OK;
}

cpu_ctl::cpu_ctl()
{
}

cause::stype cpu_ctl::init()
{
	cause::stype r = threads[0].init();
	if (is_fail(r))
		return r;

	r = mps.load();
	if (is_fail(r))
		return r;

	return cause::OK;
}

cause::stype cpu_ctl::load()
{
	native::gdt_ptr64 gdtptr;
	gdtptr.set(sizeof threads[0].gdt, &threads[0].gdt);
	native::lgdt(&gdtptr);

	native::set_ds(GDT::kern_data_offset());
	native::set_es(GDT::kern_data_offset());
	native::set_fs(GDT::kern_data_offset());
	native::set_gs(GDT::kern_data_offset());
	native::set_ss(GDT::kern_data_offset());

	native::ltr(GDT::tss_offset());

	return cause::OK;
}


// cpu_ctl::thread


cause::stype cpu_ctl::thread::init()
{
	gdt.null_entry.set_null();
	gdt.kern_code.set(0);
	gdt.kern_data.set(0);
	gdt.user_code.set(3);
	gdt.user_data.set(3);
	gdt.tss.set(&tss, sizeof tss, 0);

	mem_fill(sizeof tss, 0, &tss);
	tss.iomap_base = sizeof tss;

	return cause::OK;
}


namespace arch {

void halt() {
	native::hlt();
}

}  // namespace arch
