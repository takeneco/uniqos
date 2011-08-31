/// @file  arch.hh
/// @brief x86_64 hardware parameters.
//
// (C) 2010-2011 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_ARCH_HH_
#define ARCH_X86_64_INCLUDE_ARCH_HH_

#include "basic_types.hh"


namespace arch {

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

namespace page {

enum TYPE {
	// page type of arch::page::alloc()
	L1 = 0,
	L2,
	L3,
	L4,
	L5,

	HIGHEST = L5,
	PHYS_L1 = L1,
	PHYS_L2 = L3,
	PHYS_L3 = L5,
};
/// page size params.
enum {
	PHYS_L1_SIZE_BITS = 12,
	PHYS_L2_SIZE_BITS = 21,
	PHYS_L3_SIZE_BITS = 30,

	PHYS_L1_SIZE      = 1 << PHYS_L1_SIZE_BITS,
	PHYS_L2_SIZE      = 1 << PHYS_L2_SIZE_BITS,
	PHYS_L3_SIZE      = 1 << PHYS_L3_SIZE_BITS,

	L1_SIZE_BITS = PHYS_L1_SIZE_BITS,     // 12
	L2_SIZE_BITS = PHYS_L1_SIZE_BITS + 6, // 18
	L3_SIZE_BITS = PHYS_L2_SIZE_BITS,     // 21
	L4_SIZE_BITS = PHYS_L2_SIZE_BITS + 6, // 27
	L5_SIZE_BITS = PHYS_L3_SIZE_BITS,     // 30

	L1_SIZE      = 1 << L1_SIZE_BITS,     // 4KiB
	L2_SIZE      = 1 << L2_SIZE_BITS,     // 256KiB
	L3_SIZE      = 1 << L3_SIZE_BITS,     // 2MiB
	L4_SIZE      = 1 << L4_SIZE_BITS,     // 128MiB
	L5_SIZE      = 1 << L5_SIZE_BITS,     // 1GiB
};

cause::stype alloc(TYPE page_type, uptr* padr);
cause::stype free(TYPE page_type, uptr padr);

}  // namespace page

inline void* map_phys_mem(uptr padr_from, uptr size) {
	size = size;
	return reinterpret_cast<void*>(PHYSICAL_MEMMAP_BASEADR + padr_from);
}
inline void unmap_phys_mem(void* vadr) {
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
