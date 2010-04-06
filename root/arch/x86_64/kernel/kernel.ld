/* @file    arch/x86_64/kernel/kernel.ld
 * @author  Kato Takeshi
 * @brief   カーネル本体のリンカスクリプト。
 *
 * (C) 2009-2010 Kato Takeshi.
 */

OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS {
	. = 0xffffffff00000000;
	kern_head_addr = .;
	.text : { *(.text.entry) *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	. = 0xffffffffffff8000;
	.bss : { *(.bss) }
	kern_tail_addr = .;
}
