// @file    arch/x86_64/include/native.hh
// @author  Kato Takeshi
// @brief   C言語から呼び出すアセンブラ命令
//
// (C) 2010 Kato Takeshi.

#ifndef _ARCH_X86_64_INCLUDE_NATIVE_HH
#define _ARCH_X86_64_INCLUDE_NATIVE_HH

#include "btypes.hh"


inline void native_hlt() {
	asm volatile ("hlt");
}
inline void native_outb(u8 data, u16 port) {
	asm volatile ("outb %0,%1" : : "a"(data), "dN"(port));
}
inline void native_outw(u16 data, u16 port) {
	asm volatile ("outw %0,%1" : : "a"(data), "dN"(port));
}
inline void native_outl(u32 data, u16 port) {
	asm volatile ("outl %0,%1" : : "a"(data), "dN"(port));
}
inline u8 native_inb(u16 port) {
	u8 data;
	asm volatile ("inb %1,%0" : "=a"(data) : "dN"(port));
	return data;
}
inline u16 native_inw(u16 port) {
	u16 data;
	asm volatile ("inw %1,%0" : "=a"(data) : "dN"(port));
	return data;
}
inline u32 native_inl(u16 port) {
	u32 data;
	asm volatile ("inl %1,%0" : "=a"(data) : "dN"(port));
	return data;
}
inline void native_cli() {
	asm volatile ("cli");
}
inline void native_sti() {
	asm volatile ("sti");
}
inline u32 native_get_cr0_32() {
	u32 cr0;
	asm volatile ("movl %%cr0, %0" : "=r" (cr0));
	return cr0;
}
inline void native_set_cr0_32(u32 cr0) {
	asm volatile ("movl %0, %%cr0" : : "r" (cr0));
}
inline u64 native_get_cr0_64() {
	u64 cr0;
	asm volatile ("movq %%cr0, %0" : "=r" (cr0));
	return cr0;
}
inline void native_set_cr0_64(u64 cr0) {
	asm volatile ("movq %0, %%cr0" : : "r" (cr0));
}
inline u64 native_get_cr3() {
	u64 cr3;
	asm volatile ("movq %%cr3, %0" : "=r" (cr3));
	return cr3;
}
inline void native_set_cr3(u64 cr3) {
	asm volatile ("movq %0, %%cr3" : : "r" (cr3));
}
inline s16 native_bsfw(u16 data) {
	s16 index;
	asm volatile ("bsfw %1, %0" : "=r" (index) : "rm" (data));
	return index;
}
inline s32 native_bsfl(u32 data) {
	s32 index;
	asm volatile ("bsfl %1, %0" : "=r" (index) : "rm" (data));
	return index;
}
inline s64 native_bsfq(u64 data) {
	u64 index;
	asm volatile ("bsfq %1, %0" : "=r" (index) : "rm" (data));
	return index;
}
inline s16 native_bsrw(u16 data) {
	u16 index;
	asm volatile ("bsrw %1, %0" : "=r" (index) : "rm" (data));
	return index;
}
inline s32 native_bsrl(u32 data) {
	u32 index;
	asm volatile ("bsrl %1, %0" : "=r" (index) : "rm" (data));
	return index;
}
inline s64 native_bsrw(u64 data) {
	u64 index;
	asm volatile ("bsrq %1, %0" : "=r" (index) : "rm" (data));
	return index;
}
inline s16 bitscan_forward_16(u16 data) {
	return data != 0 ? native_bsfw(data) : -1;
}
inline s32 bitscan_forward_32(u32 data) {
	return data != 0 ? native_bsfl(data) : -1;
}
inline s64 bitscan_forward_64(u64 data) {
	return data != 0 ? native_bsfq(data) : -1;
}
inline s16 bitscan_reverse_16(u16 data) {
	return data != 0 ? native_bsrw(data) : -1;
}
inline s32 bitscan_reverse_32(u32 data) {
	return data != 0 ? native_bsrl(data) : -1;
}
inline s64 bitscan_reverse_64(u64 data) {
	return data != 0 ? native_bsrq(data) : -1;
}

// セグメントレジスタの値を返す。
// デバッグ用。
inline u16 get_cs() {
	u16 cs;
	asm volatile (
	    "movw %%cs, %%ax \n"
	    "movw %%ax, %0" : "=rm" (cs) : : "%ax");
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
inline u16 native_get_ss() {
	u16 ss;
	asm volatile (
	    "movw %%ss, %%ax \n"
	    "movw %%ax, %0" : "=rm" (ss) : : "%ax");
	return ss;
}
inline void native_set_ss(u16 ss) {
	asm volatile (
	    "movw %0, %%ax \n"
	    "movw %%ax, %%ss" : : "rm" (ss) : "%ax");
}

// @brief Global/Interrupt Descriptor Table register.
//
// アライメント調整を防ぐために ptr を分割した。
struct gidt_ptr64 {
	u16 length;
	u16 ptr[4];

	void init(
		u16   len,    ///< sizeof gdt/idt table
		void* table)  ///< Phisical address to gdt/idt table
	{
		length = len - 1;
		const u64 p = reinterpret_cast<u64>(table);
		ptr[0] = static_cast<u16>(p);
		ptr[1] = static_cast<u16>(p >> 16);
		ptr[2] = static_cast<u16>(p >> 32);
		ptr[3] = static_cast<u16>(p >> 48);
	}
};
typedef gidt_ptr64 gdt_ptr64;
typedef gidt_ptr64 idt_ptr64;

// @brief lgdt を実行する。
inline void native_lgdt(gdt_ptr64* ptr) {
	asm volatile ("lgdt %0" : : "m"(*ptr));
}

// @brief lidt を実行する。
inline void native_lidt(idt_ptr64* ptr) {
	asm volatile ("lidt %0" : : "m"(*ptr));
}


#endif  // Include guard.
