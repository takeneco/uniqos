// @file  misc.hh
//
// (C) 2010-2011 Kato Takeshi
//

#ifndef ARCH_X86_64_KERNEL_SETUP_MISC_HH_
#define ARCH_X86_64_KERNEL_SETUP_MISC_HH_

#include "btypes.hh"
#include "log.hh"


// memory alloc
void  memory_init();
void* memory_alloc(uptr size, uptr align = 8);
void  memory_free(void* p);

struct setup_memory_dumpdata;
int   freemem_dump(setup_memory_dumpdata* dumpto, int n);
int   nofreemem_dump(setup_memory_dumpdata* dumpto, int n);

void* memory_move(void* dest, const void* src, uptr size);

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


/// setup log
class on_memory_log : public kernel_log
{
public:
	on_memory_log();

private:
	static void write(kernel_log* , const void* data, u32 bytes);
};


// LZMA decode wrapper
u64 lzma_decode_size(const u8* src);
bool lzma_decode(
    u8*  src,
    uptr src_len,
    u8*  dest,
    uptr dest_len);


// HPET
bool hpet_init();
void hpet_uninit();
void timer_sleep(u32 msecs);


#endif  // Include guard

