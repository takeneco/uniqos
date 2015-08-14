/// @file native_ops.hh
/// @brief  C++ から呼び出すアセンブラ命令

//  Uniqos  --  Unique Operating System
//  (C) 2010 KATO Takeshi
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

#ifndef ARCH_X86_64_INCLUDE_NATIVE_OPS_HH_
#define ARCH_X86_64_INCLUDE_NATIVE_OPS_HH_

#include <core/basic.hh>


namespace native {

inline void hlt() {
	asm volatile ("hlt");
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

inline void write_msr(u64 val, u32 adr)
{
	asm volatile ("wrmsr" : :
	    "d" (static_cast<u32>(val >> 32)),
	    "a" (static_cast<u32>(val & 0xffffffff)),
	    "c" (adr));
}

inline u64 read_msr(u32 adr)
{
	u32 r1, r2;
	asm volatile ("rdmsr" : "=d" (r2), "=a" (r1) : "c" (adr));

	return u64(r2) << 32 | u64(r1);
}

}  // namespace native


#endif  // include guard

