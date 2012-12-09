/// @file native_ops.hh
/// @brief  C++ から呼び出すアセンブラ命令
//
// (C) 2010-2012 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_NATIVE_OPS_HH_
#define ARCH_X86_64_INCLUDE_NATIVE_OPS_HH_

#include <basic.hh>


namespace native {

inline u8 mem_read8(const u8* ptr) {
	u8 r;
	asm volatile ("movb %1, %0" : "=r" (r) : "m" (*ptr));
	return r;
}
inline u8 mem_read8(uptr adr) {
	return mem_read8(reinterpret_cast<const u8*>(adr));
}
inline u16 mem_read16(const u16* ptr) {
	u16 r;
	asm volatile ("movw %1, %0" : "=r" (r) : "m" (*ptr));
	return r;
}
inline u16 mem_read16(uptr adr) {
	return mem_read16(reinterpret_cast<const u16*>(adr));
}
inline u32 mem_read32(const u32* ptr) {
	u32 r;
	asm volatile ("movl %1, %0" : "=r" (r) : "m" (*ptr));
	return r;
}
inline u32 mem_read32(uptr adr) {
	return mem_read32(reinterpret_cast<const u32*>(adr));
}
inline u64 mem_read64(const u64* ptr) {
	u64 r;
	asm volatile ("movq %1, %0" : "=r" (r) : "m" (*ptr));
	return r;
}
inline u64 mem_read64(uptr adr) {
	return mem_read64(reinterpret_cast<const u64*>(adr));
}
inline void mem_write8(u8 x, u8* ptr) {
	asm volatile ("movb %1, %0" : "=m" (*ptr) : "r" (x));
}
inline void mem_write8(u8 x, uptr adr) {
	mem_write8(x, reinterpret_cast<u8*>(adr));
}
inline void mem_write16(u16 x, u16* ptr) {
	asm volatile ("movw %1, %0" : "=m" (*ptr) : "r" (x));
}
inline void mem_write16(u16 x, uptr adr) {
	mem_write16(x, reinterpret_cast<u16*>(adr));
}
inline void mem_write32(u32 x, u32* ptr) {
	asm volatile ("movl %1, %0" : "=m" (*ptr) : "r" (x));
}
inline void mem_write32(u32 x, uptr adr) {
	mem_write32(x, reinterpret_cast<u32*>(adr));
}
inline void mem_write64(u64 x, u64* ptr) {
	asm volatile ("movq %1, %0" : "=m" (*ptr) : "r" (x));
}
inline void mem_write64(u64 x, uptr adr) {
	mem_write64(x, reinterpret_cast<u64*>(adr));
}
inline void hlt() {
	asm volatile ("hlt");
}
inline void outb(u8 data, u16 port) {
	asm volatile ("outb %0,%1" : : "a" (data), "dN" (port));
}
inline void outw(u16 data, u16 port) {
	asm volatile ("outw %0,%1" : : "a" (data), "dN" (port));
}
inline void outl(u32 data, u16 port) {
	asm volatile ("outl %0,%1" : : "a" (data), "dN" (port));
}
inline u8 inb(u16 port) {
	u8 data;
	asm volatile ("inb %1,%0" : "=a" (data) : "dN" (port));
	return data;
}
inline u16 inw(u16 port) {
	u16 data;
	asm volatile ("inw %1,%0" : "=a" (data) : "dN" (port));
	return data;
}
inline u32 inl(u16 port) {
	u32 data;
	asm volatile ("inl %1,%0" : "=a" (data) : "dN" (port));
	return data;
}
inline void cli() {
	asm volatile ("cli");
}
inline void sti() {
	asm volatile ("sti");
}
inline u32 get_cr0_32() {
	u32 cr0;
	asm volatile ("movl %%cr0, %0" : "=r" (cr0));
	return cr0;
}
inline void set_cr0_32(u32 cr0) {
	asm volatile ("movl %0, %%cr0" : : "r" (cr0));
}
inline u64 get_cr0_64() {
	u64 cr0;
	asm volatile ("movq %%cr0, %0" : "=r" (cr0));
	return cr0;
}
inline void set_cr0_64(u64 cr0) {
	asm volatile ("movq %0, %%cr0" : : "r" (cr0));
}
inline u64 get_cr3() {
	u64 cr3;
	asm volatile ("movq %%cr3, %0" : "=r" (cr3));
	return cr3;
}
inline void set_cr3(u64 cr3) {
	asm volatile ("movq %0, %%cr3" : : "r" (cr3));
}
inline u64 get_cr4_64() {
	u64 cr4;
	asm volatile ("movq %%cr4, %0" : "=r" (cr4));
	return cr4;
}
inline void set_cr4(u64 cr4) {
	asm volatile ("movq %0, %%cr4" : : "r" (cr4));
}
inline u64 get_ef_64() {
	u64 ef;
	asm volatile ("pushfq\n"
	              "popq %0" : "=r" (ef) : : "memory");
	return ef;
}

// セグメントレジスタの値を返す。
// デバッグ用。
inline u16 get_cs() {
	u16 cs;
	asm volatile ("movw %%cs, %0" : "=r" (cs));
	return cs;
}
inline u16 get_ds() {
	u16 ds;
	asm volatile (
	    "movw %%ds, %%ax \n"
	    "movw %%ax, %0" : "=rm" (ds) : : "%ax");
	return ds;
}
inline u16 get_fs() {
	u16 fs;
	asm volatile (
	    "movw %%fs, %%ax \n"
	    "movw %%ax, %0" : "=rm" (fs) : : "%ax");
	return fs;
}
inline u16 get_gs() {
	u16 gs;
	asm volatile (
	    "movw %%gs, %%ax \n"
	    "movw %%ax, %0" : "=rm" (gs) : : "%ax");
	return gs;
}
inline u16 get_ss() {
	u16 ss;
	asm volatile (
	    "movw %%ss, %%ax \n"
	    "movw %%ax, %0" : "=rm" (ss) : : "%ax");
	return ss;
}
inline void set_ds(u16 ds) {
	asm volatile ("movw %0, %%ds" : : "r" (ds));
}
inline void set_es(u16 es) {
	asm volatile ("movw %0, %%es" : : "r" (es));
}
inline void set_fs(u16 fs) {
	asm volatile ("movw %0, %%fs" : : "r" (fs));
}
inline void set_gs(u16 gs) {
	asm volatile ("movw %0, %%gs" : : "r" (gs));
}
inline void set_ss(u16 ss) {
	asm volatile ("movw %0, %%ss" : : "r" (ss));
}
inline void ltr(u16 tr) {
	asm volatile ("ltr %0" : : "r" (tr));
}

// @brief Global/Interrupt Descriptor Table register.
//
// アライメント調整を防ぐために ptr を分割した。
struct gidt_ptr64 {
	u16 length;
	u16 ptr[4];

	void set(u16   len,    ///< sizeof gdt/idt table
	         void* table)  ///< Phisical address to gdt/idt table
	{
		length = len - 1;
		const u64 p = reinterpret_cast<u64>(table);
		ptr[0] = static_cast<u16>(p);
		ptr[1] = static_cast<u16>(p >> 16);
		ptr[2] = static_cast<u16>(p >> 32);
		ptr[3] = static_cast<u16>(p >> 48);
	}

	void get(u16*   len,    ///< sizeof gdt/idt table
	         void** table)  ///< Phisical address to gdt/idt table
	{
		*len = length;
		*table = reinterpret_cast<void*>(
		    (static_cast<u64>(ptr[0])) |
		    (static_cast<u64>(ptr[1]) << 16) |
		    (static_cast<u64>(ptr[2]) << 32) |
		    (static_cast<u64>(ptr[3]) << 48));
	}
};
typedef gidt_ptr64 gdt_ptr64;
typedef gidt_ptr64 idt_ptr64;

inline void lgdt(gdt_ptr64* ptr) {
	asm volatile ("lgdt %0" : : "m" (*ptr));
}
inline void sgdt(gdt_ptr64* ptr) {
	asm volatile ("sgdt %0" : "=m" (*ptr));
}
inline void lidt(idt_ptr64* ptr) {
	asm volatile ("lidt %0" : : "m" (*ptr));
}

inline void write_msr(u32 adr, u64 val)
{
	asm volatile ("wrmsr" : :
	    "d" (val >> 32), "a" (val & 0xffffffff), "c" (adr));
}

inline u64 read_msr(u32 adr)
{
	u32 r1, r2;
	asm volatile ("rdmsr" : "=d" (r2), "=a" (r1) : "c" (adr));

	return u64(r2) << 32 | u64(r1);
}

}  // namespace native


#endif  // include guard

