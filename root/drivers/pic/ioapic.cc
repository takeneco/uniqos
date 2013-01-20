/// @file  ioapic.cc
/// @brief I/O APIC driver.

//  UNIQOS  --  Unique Operating System
//  (C) 2013 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <spinlock.hh>
#include <arch.hh>
#include <irq_src.hh>
#include <mempool.hh>
#include <native_ops.hh>
#include <new_ops.hh>


void lapic_eoi();

namespace {

class ioapic : public pic_device
{
	DISALLOW_COPY_AND_ASSIGN(ioapic);

	struct memmapped_regs;

public:
	ioapic(operations* _ops, uptr reg_padr) :
		pic_device(_ops),
		regs(static_cast<memmapped_regs*>(
		    arch::map_phys_adr(reg_padr, sizeof (memmapped_regs))))
	{}

	cause::type on_pic_device_enable(uint irq, arch::intr_id vec);
	cause::type on_pic_device_disable(uint irq);
	void on_pic_device_eoi();

private:
	void write_reg(u32 sel, u32 data);

private:
	struct memmapped_regs {
		u32 ioregsel;
		u32 unused[3];
		u32 iowin;
	};
	memmapped_regs* regs;

	spin_lock reg_lock;
};

cause::type ioapic::on_pic_device_enable(uint irq, arch::intr_id vec)
{
	cpu_id cpuid = 0;
	u32 flags = 0;

	// edge trigger, physical destination, fixd delivery
	write_reg(0x10 + irq * 2 + 1,
	          (static_cast<u32>(cpuid) << 24) | flags);
	write_reg(0x10 + irq * 2, vec);

	return cause::OK;
}

cause::type ioapic::on_pic_device_disable(uint irq)
{
	write_reg(0x10 + irq * 2, 0x00010000);

	return cause::OK;
}

void ioapic::on_pic_device_eoi()
{
	lapic_eoi();
}

void ioapic::write_reg(u32 sel, u32 data)
{
	reg_lock.lock();

	native::mem_write32(sel, &regs->ioregsel);
	native::mem_write32(data, &regs->iowin);

	reg_lock.unlock();
}

}  // namespace

#include <cpu_ctl.hh>
#include <mpspec.hh>
#include <global_vars.hh>
#include <irq_ctl.hh>

cause::type ioapic_setup()
{
	// search APIC
	const mpspec* mps = global_vars::arch.cpu_ctl_common_obj->get_mpspec();
	mpspec::ioapic_iterator itr(mps);

	const mpspec::ioapic_entry* ioapic_ent = itr.get_next();
	if (ioapic_ent == 0) {
		//log()("I/O APIC not found.")();
		return cause::NOT_FOUND;
	}

	// create pic_device::operations
	auto ops = new
	    (mem_alloc(sizeof (pic_device::operations)))
	    pic_device::operations;
	if (!ops)
		return cause::NOMEM;

	ops->init();
	ops->enable = pic_device::call_on_pic_device_enable<ioapic>;
	ops->disable = pic_device::call_on_pic_device_disable<ioapic>;
	ops->eoi = pic_device::call_on_pic_device_eoi<ioapic>;

	// create ioapic
	ioapic* x = new
	    (mem_alloc(sizeof (ioapic)))
	    ioapic(ops, ioapic_ent->ioapic_map_padr);
	if (!x)
		return cause::NOMEM;

	cause::type r = global_vars::arch.irq_ctl_obj->set_pic(x);
	if (is_fail(r))
		return r;

	return cause::OK;
}

