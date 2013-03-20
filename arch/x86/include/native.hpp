/* FILE : arch/x86/include/asmfunc.hpp
 * VER  : 0.0.3
 * LAST : 2009-06-30
 * (C) T.Kato 2009
 *
 * C言語から呼び出すアセンブラ命令
 */

#ifndef _ARCH_X86_INCLUDE_ASMFUNC_HPP
#define _ARCH_X86_INCLUDE_ASMFUNC_HPP

#include <btypes.hpp>

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
static inline _u32 native_get_cr0() {
	int cr0;
	asm volatile ("movl %%cr0, %0" : "=r" (cr0));
	return cr0;
}
static inline void native_set_cr0(_u32 cr0) {
	asm volatile ("movl %0, %%cr0" : : "r" (cr0));
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

/*
 * GDT/IDT
 */

struct gdtidt_ptr {
	_u16 len;
	_u16 ptr1, ptr2;
};

// gdtidt_ptr を初期化する。
static void init_gdtidt_ptr(gdtidt_ptr* ptr, _u16 len, void* table)
{
	ptr->len = len - 1;
	const _u32 p = reinterpret_cast<_u32>(table);
	ptr->ptr1 = static_cast<_u16>(p);
	ptr->ptr2 = static_cast<_u16>(p >> 16);
}

// GDT

// lgdt を実行する。
// ptr->ptr1, ptr->ptr2 のポインタは static であること。
static inline void native_lgdt(gdtidt_ptr* ptr) {
	asm volatile ("lgdt %0" : : "m"(*ptr));
}

// IDT

// lidt を実行する。
// ptr->ptr1, ptr->ptr2 のポインタは static であること。
static inline void native_lidt(gdtidt_ptr* ptr) {
	asm volatile ("lidt %0" : : "m"(*ptr));
}

#endif // _ARCH_X86_INCLUDE_ASMFUNC_HPP
