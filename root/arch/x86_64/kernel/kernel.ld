/**
 * @file    arch/x86_64/kernel/kernel.ld
 * @version 0.0.0.1
 * @author  Kato.T
 * @brief   カーネル本体のリンカスクリプト。
 */
/* (C) Kato.T 2009-2010 */

OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386:x86-64)
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
