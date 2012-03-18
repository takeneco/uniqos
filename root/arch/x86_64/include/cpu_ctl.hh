/// @file  cpu_ctl.hh
//
// (C) 2011 KATO Takeshi
//

#ifndef ARCH_X86_64_KERNEL_CPU_CTL_HH_
#define ARCH_X86_64_KERNEL_CPU_CTL_HH_

#include "basic_types.hh"
#include "cpu_idte.hh"
#include "mpspec.hh"
#include "regset.hh"


class thread;
class processor;
namespace arch {

processor* get_current_cpu();

}  // namespace arch


class cpu_share
{
public:
	cpu_share();

	cause::stype init();

	const mpspec* get_mpspec() const {
		return &mps;
	}

private:
	mpspec mps;
};

// call by cpu_ctl::IDT
void intr_init(idte* idt);

namespace arch {

/// Architecture dependent part of processor control.
class cpu_ctl
{
public:
	class IDT;

	cause::stype init();
	cause::stype load(cpu_share* sh);

	void set_running_thread(thread* t);

	const cpu_share* get_shared() { return shared; }

	void set_original_lapic_id(u8 id) { original_lapic_id = id; }
	u8 get_original_lapic_id() const { return original_lapic_id; }

private:
	void* write_ist_layout(void* mem);

public:
	/// Global Descriptor Table Entry
	struct gdte
	{
	public:
		typedef u64 type;

		enum FLAGS {
			/// Exec and Read segment
			XR  = U64(0x1a) << 40,
			/// Read and Write segment
			RW  = U64(0x12) << 40,

			/// Descriptor exist if set.
			P   = U64(1) << 47,
			/// Software useable.
			AVL = U64(1) << 52,
			/// Long mode if set (code/data).
			L   = U64(1) << 53,
			/// If long mode, then clear (code/data).
			D   = U64(1) << 54,
			/// Limit scale 4096 times if set.
			G   = U64(1) << 55,
		};

	public:
		gdte() {}
		void set_null() {
			e = 0;
		}
		void set(type base, type limit, type dpl, type flags) {
			e = (base  & 0x00ffffff) << 16 |
			    (base  & 0xff000000) << 32 |
			    (limit & 0x0000ffff)       |
			    (limit & 0x000f0000) << 32 |
			    (dpl & 3) << 45 |
			    flags;
		}

	public:
		type e;
	};

	class gdte2
	{
	public:
		typedef u64 type;

		enum FLAGS {
			/// LDT
			LDT = U64(0x02) << 40,
			/// TSS
			TSS = U64(0x09) << 40,

			/// Descriptor exist if set.
			P   = gdte::P,
			/// Software useable.
			AVL = gdte::AVL,
			/// Limit scale 4096 times if set.
			G   = gdte::G,
		};

	public:
		gdte2() {}
		void set(type base, type limit, type dpl, type flags) {
			e1.set(base, limit, dpl, flags);
			e2 = (base & U64(0xffffffff00000000)) >> 32;
		}

	public:
		gdte e1;
		type e2;
	};

	class code_seg_desc : gdte
	{
	public:
		code_seg_desc() : gdte() {}

		/// flags には AVL を指定できる。
		void set(type dpl, type flags=0) {
			// base and limit is disabled in long mode.
			gdte::set(0, 0, dpl, XR | P | L | flags);
		}
	};

	class data_seg_desc : gdte
	{
	public:
		data_seg_desc() : gdte() {}

		/// flags には AVL を指定できる。
		void set(type dpl, type flags=0) {
			// base and limit is disabled in long mode.
			gdte::set(0, 0, dpl, RW | P | L | flags);
		}
	};

	class tss_desc : gdte2
	{
	public:
		tss_desc() : gdte2() {}

		void set(type base, type limit, type dpl, type flags=0) {
			gdte2::set(base, limit, dpl, TSS | P | flags);
		}
		void set(void* base, type limit, type dpl, type flags=0) {
			set(reinterpret_cast<type>(base), limit, dpl, flags);
		}
	};

	/// Global Descriptor Table
	class GDT
	{
		static GDT* nullgdt() { return 0; }
		static u16 offset_cast(void* p) {
			return static_cast<u16>(reinterpret_cast<uptr>(p));
		}

	public:
		gdte          null_entry;
		code_seg_desc kern_code;
		data_seg_desc kern_data;
		code_seg_desc user_code;
		data_seg_desc user_data;
		tss_desc      tss;

		static u16 kern_code_offset() {
			return offset_cast(&nullgdt()->kern_code);
		}
		static u16 kern_data_offset() {
			return offset_cast(&nullgdt()->kern_data);
		}
		static u16 tss_offset() {
			return offset_cast(&nullgdt()->tss);
		}
	};

	// Task State Segment
	struct TSS
	{
		void set_ist(void* adr, int i) {
			_set_ist(reinterpret_cast<u64>(adr),
			    ist_array[i * 2], ist_array[i * 2 + 1]);
		}
		static void _set_ist(u64 adr, u32& l, u32& h) {
			l = adr & 0xffffffff;
			h = (adr >> 32) & 0xffffffff;
		}
		const void* get_ist(int i) const {
			return _get_ist(ist_array[i * 2], ist_array[i * 2 + 1]);
		}
		void* get_ist(int i) {
			return _get_ist(ist_array[i * 2], ist_array[i * 2 + 1]);
		}
		static void* _get_ist(u32 l, u32 h) {
			return reinterpret_cast<void*>(
			    u64(h) << 32 |
			    u64(l));
		}
		u32 reserved_1;
		u32 rsp0l;
		u32 rsp0h;
		u32 rsp1l;
		u32 rsp1h;
		u32 rsp2l;
		u32 rsp2h;
		union {
			struct {
				u32 reserved_2;
				u32 reserved_3;
				u32 ist1l;
				u32 ist1h;
				u32 ist2l;
				u32 ist2h;
				u32 ist3l;
				u32 ist3h;
				u32 ist4l;
				u32 ist4h;
				u32 ist5l;
				u32 ist5h;
				u32 ist6l;
				u32 ist6h;
				u32 ist7l;
				u32 ist7h;
			};
			u32 ist_array[16];
		};
		u32 reserved_4;
		u32 reserved_5;
		u16 reserved_6;
		u16 iomap_base;
	};
	enum {
		IST_INTR = 1,
		IST_TRAP = 2,
	};

	GDT gdt;
	TSS tss;

	arch::regset* running_thread_regset;
	cpu_share* shared;

	u8 original_lapic_id;
};


// 別ヘッダへ移動予定
class cpu_ctl::IDT
{
public:
	void init() {
		intr_init(idt);
	}

private:
	idte idt[256];
};

}  // namespace arch


#endif  // include guard

