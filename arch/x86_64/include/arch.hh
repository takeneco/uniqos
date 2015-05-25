/// @file  arch.hh
/// @brief x86_64 hardware parameters.

//  Uniqos  --  Unique Operating System
//  (C) 2010-2015 KATO Takeshi
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

#ifndef ARCH_X86_64_INCLUDE_ARCH_HH_
#define ARCH_X86_64_INCLUDE_ARCH_HH_

#include <core/basic.hh>


#if ARCH_ADR_BITS == 64
#	define ARCH_PROVIDES_MEM_FILL
#endif  // ARCH_ADR_BITS == 64

namespace arch {

typedef u32 _cpu_id;
typedef u64 _cpu_word;

typedef u8 intr_id;

enum {
	BITS_IN_BYTE = 8,

	// alignment
	BASIC_TYPE_ALIGN = 8,

	IRQ_CPU_OFFSET = 0x00,
	IRQ_PIC_OFFSET = 0x20,

	// Interrupt vector
	INTR_APIC_TIMER = 0x30,
};
enum {
	PHYS_MAP_ADR    = U64(0xffff800000000000),

	VADRPOOL_START  = U64(0xffffc00000000000),
	VADRPOOL_END    = U64(0xffffe00000000000),
};

inline void* map_phys_adr(uptr padr, uptr /* size */) {
	return reinterpret_cast<void*>(PHYS_MAP_ADR + padr);
}
inline void* map_phys_adr(void* padr, uptr size) {
	return map_phys_adr(reinterpret_cast<uptr>(padr), size);
}
inline uptr unmap_phys_adr(const void* vadr, uptr /* size */) {
	return reinterpret_cast<uptr>(vadr) - PHYS_MAP_ADR;
}

_cpu_id get_cpu_node_id();
_cpu_id get_cpu_lapic_id();
int get_cpu_id();

void intr_enable();
void intr_disable();
void intr_wait();

}  // namespace arch

typedef arch::_cpu_id cpu_id;
typedef arch::_cpu_id cpu_id_t;
typedef arch::_cpu_word cpu_word;
typedef arch::_cpu_word cpu_word_t;


#endif  // include guard

