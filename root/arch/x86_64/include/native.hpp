/* @file    arch/x86_64/include/native.hpp
 * @version 0.0.0.1
 * @author  Kato.T
 * @brief   C言語から呼び出すアセンブラ命令
 */
// (C) T.Kato 2010

#ifndef _ARCH_X86_64_INCLUDE_NATIVE_HPP
#define _ARCH_X86_64_INCLUDE_NATIVE_HPP

#include "btypes.hpp"

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

/*
 * GDT/IDT
 */

/**
 * @brief Global/Interrupt Descriptor Table
 * アライメント調整を防ぐために ptr を分割した。
 */
struct gidt_ptr64 {
	_u16 len;
	_u16 ptr[4];
};

/**
 * gidt_ptr64 を初期化する。
 */
inline void init_gidt_ptr64(gidt_ptr64* gidt, _u16 len, const void* table)
{
	gidt->len = len - 1;
	const _u64 p = reinterpret_cast<_u64>(table);
	gidt->ptr[0] = static_cast<_u16>(p);
	gidt->ptr[1] = static_cast<_u16>(p >> 16);
	gidt->ptr[2] = static_cast<_u16>(p >> 32);
	gidt->ptr[3] = static_cast<_u16>(p >> 48);
}

// GDT

/**
 * @brief lgdt を実行する。
 */
inline void native_lgdt(gidt_ptr64* ptr) {
	asm volatile ("lgdt %0" : : "m"(*ptr));
}

// IDT

/**
 * @brief lidt を実行する。
 */
inline void native_lidt(gidt_ptr64* ptr) {
	asm volatile ("lidt %0" : : "m"(*ptr));
}

#endif // _ARCH_X86_64_INCLUDE_NATIVE_HPP
