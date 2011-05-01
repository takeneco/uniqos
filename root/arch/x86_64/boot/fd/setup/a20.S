/**
 * @file    arch/x86_64/boot/setup/a20.S
 * @version 0.0.0.1
 * @author  Kato.T
 * @brief   A20 gate ops.
 */
// (C) Kato.T 2010

.code16

//#include "boot.h"

.globl a20_test_mem, a20_test_bios, a20_open_bios
.section .text

/*
 * Test A20 gate status by memory writing.
 * If (ffff:7c10) and (0000:7c00) is synced A20 gate is closed.
 * If A20 gate opened, %ax = $a20isopen else %ax = $a20isclose.
 *
 * Destory %ax, %bx, %fs, %gs, (0000:7c00).
 */
a20_test_mem:
	xorw  %ax, %ax
	movw  %ax, %fs
	movw  $0xffff, %ax
	movw  %ax, %gs
	// (ffff:7c10) + 1 -> (0000:7c00)
	movw  %gs:0x7c10, %bx
	movw  %bx, %ax
	incw  %ax
	movw  %ax, %fs:0x7c00
	movw  %gs:0x7c10, %ax
	cmpw  %ax, %bx
	jne   1f
	movw  $a20isopen, %ax
	jmp   2f
1:	movw  $a20isclose, %ax
2:	ret

/*
 * Test A20 gate status by bios call.
 * If A20 gate opened, %ax = $a20isopen else %ax = $a20isclose.
 *
 * Destory %ax.
 */
a20_test_bios:
	movw  $0x2402, %ax
	int   $0x15
	jc    1f  // error.
	test  %al, %al
	jz    1f
	movw  $a20isopen, %ax
	jmp   2f
1:	movw  $a20isclose, %ax
2:	ret

/*
 * Open A20 gate by bios call.
 *
 * Destory %ax.
 */
a20_open_bios:
	movw  $0x2401, %ax
	int   $0x15
	ret

