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


// I/O APIC
void* ioapic_base();

class ioapic_control
{
	struct memmapped_regs {
		u32 volatile ioregsel;
		u32          unused[3];
		u32 volatile iowin;
	};
	memmapped_regs* const regs;

public:
	ioapic_control(void* base) 
	    : regs(reinterpret_cast<memmapped_regs*>(base))
	{}
	u32 read(u32 sel) {
		regs->ioregsel = sel;
		return regs->iowin;
	}
	void write(u32 sel, u32 data) {
		regs->ioregsel = sel;
		regs->iowin = data;
	}
	void mask(u32 index) {
		write(0x10 + index * 2, 0x00010000);
	}
	void unmask(u32 index, u8 cpuid, u8 vec) {
		// edge trigger, physical destination, fixd delivery
		write(0x10 + index * 2 + 1, static_cast<u32>(cpuid) << 24);
		write(0x10 + index * 2, vec);
	}
};


// HPET
bool hpet_init();
void hpet_uninit();
void timer_sleep(u32 msecs);

extern "C" void on_interrupt_timer();


#endif  // Include guard

