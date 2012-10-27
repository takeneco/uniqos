/// @file  apic.cc
/// @brief Control APIC.

//  UNIQOS  --  Unique Operating System
//  (C) 2010-2012 KATO Takeshi
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

#include <arch.hh>
#include <global_vars.hh>
#include <intr_ctl.hh>
#include <log.hh>
#include <native_ops.hh>


namespace {

/// Local APIC Registers
enum LAPIC_REG {
	LOCAL_APIC_REG_BASE = arch::PHYS_MAP_ADR + 0xfee00000,

	/// ID
	LOCAL_APIC_ID = 0x0020,
	/// Version
	LOCAL_APIC_VERSION = 0x0030,
	/// Task Priority Register
	LOCAL_APIC_TPR = 0x0080,
	/// ?
	LOCAL_APIC_APR = 0x0090,
	/// Processor Priority Register
	LOCAL_APIC_PPR = 0x00a0,
	/// End of Interrupt
	LOCAL_APIC_EOI = 0x00b0,
	/// Logical Destination Register
	LOCAL_APIC_LDR = 0x00d0,
	/// Destination Format Register
	LOCAL_APIC_DFR = 0x00e0,
	/// Spurious Interrupt Vector Register
	LOCAL_APIC_SVR = 0x00f0,
	/// In Service Register
	LOCAL_APIC_ISR_BASE = 0x0100,
	/// Trigger Mode Register
	LOCAL_APIC_TMR_BASE = 0x0180,
	/// Interrupt Request Register
	LOCAL_APIC_IRR_BASE = 0x0200,
	/// Error Status Register
	LOCAL_APIC_ERROR    = 0x0280,
	/// Interrupt Command Register (0-31)
	LOCAL_APIC_ICR_LOW  = 0x0300,
	/// Interrupt Command Register (32-63)
	LOCAL_APIC_ICR_HIGH = 0x0310,
	/// LVT Timer Register
	LOCAL_APIC_LVT_TIMER = 0x0320,
	/// Initial Count Register (timer)
	LOCAL_APIC_INI_COUNT = 0x0380,
	/// Current Count Register (timer)
	LOCAL_APIC_CUR_COUNT = 0x0390,
	/// Divide Configuration Register (timer)
	LOCAL_APIC_DCR = 0x03e0,
};

inline u32 read_reg(LAPIC_REG offset) {
	return native::mem_read32(LOCAL_APIC_REG_BASE + offset);
}
inline void write_reg(u32 val, LAPIC_REG offset) {
	native::mem_write32(val, LOCAL_APIC_REG_BASE + offset);
}

intr_handler timer_ih;
void timer_handler(intr_handler*)
{
	write_reg(0, LOCAL_APIC_EOI);
}

cause::type local_apic_init()
{
	log()("LAPIC version:").u(read_reg(LOCAL_APIC_VERSION), 16)();

	// Local APIC enable
	u32 tmp = read_reg(LOCAL_APIC_SVR);
	write_reg(tmp | 0x100, LOCAL_APIC_SVR);

	write_reg(0, LOCAL_APIC_ID);
	write_reg(0x1000000, LOCAL_APIC_LDR);
	write_reg(0xffffffff, LOCAL_APIC_DFR);

	// Task priority lowest
	write_reg(0, LOCAL_APIC_TPR);

	//*local_apic_reg(LOCAL_APIC_DCR) = 0xb; // clock/1
	write_reg(0xa, LOCAL_APIC_DCR); // clock/128

	// one shot, unmask, とりあえずベクタ0x30
	write_reg(arch::INTR_APIC_TIMER, LOCAL_APIC_LVT_TIMER);

	timer_ih.handler = timer_handler;
	global_vars::core.intr_ctl_obj->install_handler(
		arch::INTR_APIC_TIMER, &timer_ih);

	return cause::OK;
}

void set_ipi_dest(u32 id)
{
	id <<= 24;
	write_reg(id, LOCAL_APIC_ICR_HIGH);
}

enum {
	ICR_INIT_MODE = 0x000000500,
	ICR_STARTUP_MODE = 0x00000600,

	ICR_PHYSICAL_DEST = 0x000000000,
	ICR_LOGICAL_DEST = 0x000000800,

	ICR_DEASSERT_LEVEL = 0x00000000,
	ICR_ASSERT_LEVEL = 0x00004000,

	ICR_EDGE_TRIGGER = 0x00000000,
	ICR_LEVEL_TRIGGER = 0x00008000,

	ICR_BROADCAST = 0x000c0000,
};
void post_ipi(u8 vec, u64 flags)
{
	u64 val = vec | flags;
	write_reg(val, LOCAL_APIC_ICR_LOW);
}

}  // namespace

namespace arch {

cause::type apic_init()
{
	return local_apic_init();
}

void wait(u32 n)
{
	write_reg(n, LOCAL_APIC_INI_COUNT);
	native::hlt();
}

}

void lapic_eoi()
{
	write_reg(0, LOCAL_APIC_EOI);
}

void lapic_post_init_ipi()
{
	post_ipi(0,
	    ICR_INIT_MODE |
	    ICR_ASSERT_LEVEL |
	    ICR_EDGE_TRIGGER |
	    ICR_BROADCAST);
}

void lapic_post_startup_ipi(u8 vec)
{
	post_ipi(vec,
	    ICR_STARTUP_MODE |
	    ICR_ASSERT_LEVEL |
	    ICR_EDGE_TRIGGER |
	    ICR_BROADCAST);
}

namespace arch {

int get_cpu_id()
{
	const u32 id = read_reg(LOCAL_APIC_ID);

	return id >> 24;
}

}

