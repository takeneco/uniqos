/// @file   misc.hh
//
// (C) 2011 KATO Takeshi
//

#include "memdump.hh"


struct acpi_memmap {
	enum type_value {
		MEMORY = 1,
		RESERVED = 2,
		ACPI = 3,
		NVS = 4,
		UNUSUABLE = 5
	};
	u64 base;
	u64 length;
	u32 type;
	u32 attr;
};

void mem_init();
void mem_add(u64 head, u64 bytes, bool avoid);
void* mem_alloc(uptr size, uptr align);
void mem_free(void* p);
int freemem_dump(setup_memory_dumpdata* dumpto, int n);
int nofreemem_dump(setup_memory_dumpdata* dumpto, int n);

