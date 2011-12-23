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
	PHYSICAL_MEMMAP_BASEADR = U64(0xffff800000000000),
	// Interrupt vector
	INTR_APIC_TIMER = 0x30,
};
enum {
	PHYSICAL_ADRMAP = U64(0xffff800000000000),
};

/////////////////////////////////////////////////////////////////////
// memory paging
/////////////////////////////////////////////////////////////////////

namespace page {

enum TYPE {
	INVALID = -1,

	// page type of arch::page::alloc()
	L1 = 0,
	L2,
	L3,
	L4,
	L5,
	L6,
	L7,
	HIGHEST = L5, // L6, L7 is not using in the page management.

	PHYS_L1 = L1,
	PHYS_L2 = L3,
	PHYS_L3 = L5,
	PHYS_L4 = L7,
	PHYS_HIGHEST = PHYS_L4,
};
/// page size params.
enum {
	PHYS_L1_SIZE_BITS = 12,
	PHYS_L2_SIZE_BITS = 21,
	PHYS_L3_SIZE_BITS = 30,
	PHYS_L4_SIZE_BITS = 39,

	PHYS_L1_SIZE      = U64(1) << PHYS_L1_SIZE_BITS,
	PHYS_L2_SIZE      = U64(1) << PHYS_L2_SIZE_BITS,
	PHYS_L3_SIZE      = U64(1) << PHYS_L3_SIZE_BITS,
	PHYS_L4_SIZE      = U64(1) << PHYS_L4_SIZE_BITS,

	L1_SIZE_BITS = PHYS_L1_SIZE_BITS,     // 12
	L2_SIZE_BITS = PHYS_L1_SIZE_BITS + 6, // 18
	L3_SIZE_BITS = PHYS_L2_SIZE_BITS,     // 21
	L4_SIZE_BITS = PHYS_L2_SIZE_BITS + 6, // 27
	L5_SIZE_BITS = PHYS_L3_SIZE_BITS,     // 30

	L1_SIZE      = U64(1) << L1_SIZE_BITS, // 4KiB
	L2_SIZE      = U64(1) << L2_SIZE_BITS, // 256KiB
	L3_SIZE      = U64(1) << L3_SIZE_BITS, // 2MiB
	L4_SIZE      = U64(1) << L4_SIZE_BITS, // 128MiB
	L5_SIZE      = U64(1) << L5_SIZE_BITS, // 1GiB
};

inline uptr size_of_type(TYPE pt)
{
	switch (pt) {
	case L1: return L1_SIZE;
	case L2: return L2_SIZE;
	case L3: return L3_SIZE;
	case L4: return L4_SIZE;
	case L5: return L5_SIZE;
	default: return 0;
	}
}

inline TYPE type_of_size(uptr size)
{
	if (size <= L1_SIZE)      return L1;
	else if (size <= L2_SIZE) return L2;
	else if (size <= L3_SIZE) return L3;
	else if (size <= L4_SIZE) return L4;
	else if (size <= L5_SIZE) return L5;
	else                      return INVALID;
}

cause::stype alloc(TYPE page_type, uptr* padr);
cause::stype free(TYPE page_type, uptr padr);

}  // namespace page

inline void* map_phys_adr(uptr padr, uptr /* size */) {
	return reinterpret_cast<void*>(PHYSICAL_ADRMAP + padr);
}
inline void* map_phys_adr(void* padr, uptr size) {
	return map_phys_adr(reinterpret_cast<uptr>(padr), size);
}
inline uptr unmap_phys_adr(void* vadr, uptr /* size */) {
	return reinterpret_cast<uptr>(vadr) - PHYSICAL_ADRMAP;
}

namespace pmem
{


}  // namespace pmem

void halt();

}  // namespace arch


#endif  // include guard
