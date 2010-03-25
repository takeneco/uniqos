// @file    arch/x86_64/include/native.hh
// @author  Kato Takeshi
// @brief   C言語から呼び出すアセンブラ命令
//
// (C) 2010 Kato Takeshi.

#ifndef _ARCH_X86_64_INCLUDE_NATIVE_HH
#define _ARCH_X86_64_INCLUDE_NATIVE_HH

#include "btypes.hh"


namespace arch {

static inline void native_hlt() {
	asm volatile ("hlt");
}
static inline void native_outb(_u8 data, _u16 port) {
	asm volatile ("outb %0,%1" : : "a"(data), "dN"(port));
}
static inline void native_outw(_u16 data, _u16 port) {
	asm volatile ("outw %0,%1" : : "a"(data), "dN"(port));
}
static inline void native_outl(_u32 data, _u16 port) {
	asm volatile ("outl %0,%1" : : "a"(data), "dN"(port));
}
static inline _u8 native_inb(_u16 port) {
	_u8 data;
	asm volatile ("inb %1,%0" : "=a"(data) : "dN"(port));
	return data;
}
static inline _u16 native_inw(_u16 port) {
	_u16 data;
	asm volatile ("inw %1,%0" : "=a"(data) : "dN"(port));
	return data;
}
static inline _u32 native_inl(_u16 port) {
	_u32 data;
	asm volatile ("inl %1,%0" : "=a"(data) : "dN"(port));
	return data;
}
static inline void native_cli() {
	asm volatile ("cli");
}
static inline void native_sti() {
	asm volatile ("sti");
}
static inline _u32 native_get_cr0_32() {
	_u32 cr0;
	asm volatile ("movl %%cr0, %0" : "=r" (cr0));
	return cr0;
}
static inline void native_set_cr0_32(_u32 cr0) {
	asm volatile ("movl %0, %%cr0" : : "r" (cr0));
}
inline _u64 native_get_cr0_64() {
	_u64 cr0;
	asm volatile ("movq %%cr0, %0" : "=r" (cr0));
	return cr0;
}
inline void native_set_cr0_64(_u64 cr0) {
	asm volatile ("movq %0, %%cr0" : : "r" (cr0));
}

// セグメントレジスタの値を返す。
// デバッグ用。
static inline _u16 get_cs() {
	_u16 cs;
	asm volatile (
		"movw %%cs, %%ax \n"
		"movw %%ax, %0" : "=rm" (cs));
	return cs;
}
static inline _u16 get_ds() {
	_u16 ds;
	asm volatile ("movw %0, %%ds" : "=rm" (ds));
	return ds;
}
static inline _u16 get_fs() {
	_u16 fs;
	asm volatile ("movw %0, %%fs" : "=rm" (fs));
	return fs;
}
static inline _u16 get_gs() {
	_u16 gs;
	asm volatile ("movw %0, %%gs" : "=rm" (gs));
	return gs;
}

// @brief Global/Interrupt Descriptor Table register.
//
// アライメント調整を防ぐために ptr を分割した。
struct gidt_ptr64 {
	u16 length;
	u16 ptr[4];

	void Init(
		u16 len,           ///< sizeof gdt/idt table
		const void* table) ///< Ptr to gdt/idt table
	{
		length = len - 1;
		const u64 p = reinterpret_cast<u64>(table);
		ptr[0] = static_cast<u16>(p);
		ptr[1] = static_cast<u16>(p >> 16);
		ptr[2] = static_cast<u16>(p >> 32);
		ptr[3] = static_cast<u16>(p >> 48);
	}
};
typedef gidt_ptr64 GDT_Ptr64;
typedef gidt_ptr64 IDT_Ptr64;

// @brief lgdt を実行する。
inline void NativeLGDT(GDT_Ptr64* ptr) {
	asm volatile ("lgdt %0" : : "m"(*ptr));
}

// @brief lidt を実行する。
inline void NativeLIDT(IDT_Ptr64* ptr) {
	asm volatile ("lidt %0" : : "m"(*ptr));
}

}  // namespace arch

#endif  // Include guard.
