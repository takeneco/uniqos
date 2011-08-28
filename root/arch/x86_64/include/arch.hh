/// @file  arch.hh
/// @brief x86_64 hardware parameters.
//
// (C) 2010-2011 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_ARCH_HH_
#define ARCH_X86_64_INCLUDE_ARCH_HH_

#include "basic_types.hh"


namespace arch
{

enum {
	BITS_PER_BYTE = 8,

	// alignment
	BASIC_TYPE_ALIGN = 8,

	IRQ_CPU_OFFSET = 0x00,
	IRQ_PIC_OFFSET = 0x20,

	// this is not hw param.
	PHYSICAL_MEMMAP_BASEADR = 0xffff800000000000,
	// Interrupt vector
	INTR_APIC_TIMER = 0x30,
};

/////////////////////////////////////////////////////////////////////
// memory paging
/////////////////////////////////////////////////////////////////////

enum PAGE_TYPE {
	// page type of page_alloc()
	PAGE_L1 = 0,
	PAGE_L2,
	PAGE_L3,
	PAGE_L4,
	PAGE_L5,

	PAGE_HIGHEST = PAGE_L5,
	PHYS_PAGE_L1 = PAGE_L1,
	PHYS_PAGE_L2 = PAGE_L3,
	PHYS_PAGE_L3 = PAGE_L5,
};
/// page size params.
enum {
	PHYS_PAGE_L1_SIZE_BITS = 12,
	PHYS_PAGE_L2_SIZE_BITS = 21,
	PHYS_PAGE_L3_SIZE_BITS = 30,

	PHYS_PAGE_L1_SIZE      = 1 << PHYS_PAGE_L1_SIZE_BITS,
	PHYS_PAGE_L2_SIZE      = 1 << PHYS_PAGE_L2_SIZE_BITS,
	PHYS_PAGE_L3_SIZE      = 1 << PHYS_PAGE_L3_SIZE_BITS,

	PAGE_L1_SIZE_BITS = PHYS_PAGE_L1_SIZE_BITS,     // 12
	PAGE_L2_SIZE_BITS = PHYS_PAGE_L1_SIZE_BITS + 6, // 18
	PAGE_L3_SIZE_BITS = PHYS_PAGE_L2_SIZE_BITS,     // 21
	PAGE_L4_SIZE_BITS = PHYS_PAGE_L2_SIZE_BITS + 6, // 27
	PAGE_L5_SIZE_BITS = PHYS_PAGE_L3_SIZE_BITS,     // 30

	PAGE_L1_SIZE      = 1 << PAGE_L1_SIZE_BITS,     // 4KiB
	PAGE_L2_SIZE      = 1 << PAGE_L2_SIZE_BITS,     // 256KiB
	PAGE_L3_SIZE      = 1 << PAGE_L3_SIZE_BITS,     // 2MiB
	PAGE_L4_SIZE      = 1 << PAGE_L4_SIZE_BITS,     // 128MiB
	PAGE_L5_SIZE      = 1 << PAGE_L5_SIZE_BITS,     // 1GiB
};

inline void* map_direct_mem(uptr padr_from, uptr padr_to) {
	padr_to = padr_to;
	return reinterpret_cast<void*>(PHYSICAL_MEMMAP_BASEADR + padr_from);
}
inline void unmap_direct_mem(void* vadr) {
	vadr = vadr;
}

namespace pmem
{

inline void* direct_map(uptr adr) {
	return reinterpret_cast<void*>(PHYSICAL_MEMMAP_BASEADR + adr);
}

}  // namespace pmem

void halt();

}  // namespace arch


#endif  // include guard
