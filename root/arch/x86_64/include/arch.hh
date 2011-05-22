/// @file  arch.hh
/// @brief x86_64 hardware parameters.
//
// (C) 2010 Kato Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_ARCH_HH_
#define ARCH_X86_64_INCLUDE_ARCH_HH_

#include "btypes.hh"


namespace arch
{

enum {
	BITS_PER_BYTE = 8,

	// alignment
	BASIC_TYPE_ALIGN = 8,

	PAGE_L1_SIZE_BITS = 12,
	PAGE_L1_SIZE      = 1 << PAGE_L1_SIZE_BITS, // 4KiB
	PAGE_L2_SIZE_BITS = 21,
	PAGE_L2_SIZE      = 1 << PAGE_L2_SIZE_BITS, // 2MiB
	PAGE_L3_SIZE_BITS = 30,
	PAGE_L3_SIZE      = 1 << PAGE_L3_SIZE_BITS, // 1GiB

	IRQ_CPU_OFFSET = 0x00,
	IRQ_PIC_OFFSET = 0x20,

	// this is not hw param.
	PHYSICAL_MEMMAP_BASEADR = 0xffff800000000000,
	// Interrupt vector
	INTR_APIC_TIMER = 0x30,
};

namespace pmem
{

inline void* direct_map(uptr adr) {
	return reinterpret_cast<void*>(PHYSICAL_MEMMAP_BASEADR + adr);
}
cause::stype alloc_page(u64* padr);

}  // namespace pmem

void halt();

}  // namespace arch


#endif  // include guard
