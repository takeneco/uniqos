/**
 * @file    arch/x86/kernel/kernel.ld
 * @version 0.0.1
 * @date    2009-07-29
 * @author  Kato.T
 * @brief   カーネル本体のリンカスクリプト。
 */
/* (C) Kato.T 2009 */

OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS {
	. = 0x100000;
	kern_head_addr = .;
	.text : { *(.text.entry) *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	.bss : { *(.bss) }
	kern_tail_addr = .;
}
