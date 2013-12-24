/// @file  native_cpu_node.hh
//
// (C) 2013 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_NATIVE_CPU_NODE_HH_
#define ARCH_X86_64_INCLUDE_NATIVE_CPU_NODE_HH_

#include <cpu_node.hh>
#include <cpu_idte.hh>
#include <regset.hh>


namespace x86 {

class native_thread;
struct native_cpu_buffer;

/// Architecture dependent part of processor control.
class native_cpu_node : public cpu_node
{
public:
	class IDT;

public:
	native_cpu_node(native_cpu_buffer* priv_buf);

	cause::t setup();
	cause::t start_message_loop();

	void set_original_lapic_id(u8 id) { original_lapic_id = id; }
	u8 get_original_lapic_id() const { return original_lapic_id; }
	bool original_lapic_id_is_enable() const {
		return original_lapic_id != (u8)-1;
	}
	//TODO:この関数はなくしたい
	void set_running_thread(thread* t) { load_running_thread(t); }
	void load_running_thread(thread* t);

	void preempt_disable();
	void preempt_enable();

	void ready_messenger();
	void ready_messenger_np();
	void switch_messenger_after_intr();

	void post_intr_message(message* ev);
	void post_soft_message(message* ev);

	void sleep_current_thread();
	bool force_switch_thread();
	void switch_thread_after_intr(native_thread* t);

private:
	cause::t setup_tss();
	cause::t setup_gdt();
	cause::t setup_syscall();
	void* ist_layout(void* mem);

	u8   inc_preempt_disable() { return ++preempt_disable_cnt; }
	u8   dec_preempt_disable() { return --preempt_disable_cnt; }

	void message_loop();
	static void message_loop_entry(void* _cpu_node);

private:
	/// Global Descriptor Table Entry
	struct gdte
	{
	public:
		typedef u64 type;

		enum FLAGS {
			/// Exec and Read segment
			XR  = U64(0x1b) << 40,
			/// Read and Write segment
			RW  = U64(0x13) << 40,

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
		data_seg_desc user_data;
		code_seg_desc user_code;
		tss_desc      tss;

		static u16 kern_code_offset() {
			return offset_cast(&nullgdt()->kern_code);
		}
		static u16 kern_data_offset() {
			return offset_cast(&nullgdt()->kern_data);
		}
		static u16 user_code_offset() {
			return offset_cast(&nullgdt()->user_code);
		}
		static u16 user_data_offset() {
			return offset_cast(&nullgdt()->user_data);
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

	native_thread* message_thread;

	/// 外部割込みによって発生したイベントを溜める。
	/// intr_evq を操作するときは CPU が割り込み禁止状態になっていなければ
	/// ならない。
	message_queue intr_msgq;

	message_queue soft_msgq;

#if CONFIG_PREEMPT
	u8 preempt_disable_cnt;
#endif  // CONFIG_PREEMPT

	native_cpu_buffer* private_buffer;
public:

	struct {
		uptr thread_private_info;
		uptr tmp;
	} syscall_buf;

	struct {
		arch::regset* running_thread_regset;
	} intr_buf;

	u8 original_lapic_id;
};


// TODO:別ヘッダへ移動予定
class native_cpu_node::IDT
{
public:
	cause::t init() {
		return intr_init(idt);
	}

	idte* get() { return idt; }

private:
	idte idt[256];
};

// TODO:
struct native_cpu_buffer
{
	native_cpu_buffer() :
		node(this)
	{}

	arch::regset* running_thread_regset;
	uptr tmp;

	native_cpu_node node;
};

native_cpu_node* get_native_cpu_node();
native_cpu_node* get_native_cpu_node(cpu_id cpuid);

}  // namespace x86

/*
/// CPUの共有データ
class cpu_ctl_common
{
public:
	cpu_ctl_common();

	cause::t init();

	cause::t setup_idt();

	const mpspec* get_mpspec() const {
		return &mps;
	}

private:
	mpspec mps;

	arch::cpu_ctl::IDT idt;
};
*/

#endif  // include guard

