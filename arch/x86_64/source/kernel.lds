/* @file    arch/x86_64/kernel/kernel.ld
 * @brief   カーネル本体のリンカスクリプト。
 *
 * (C) 2009-2015 KATO Takeshi
 */

OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS {
	. = 0xffffffff00000000 + SIZEOF_HEADERS;
	kern_head_addr = .;
	.text : { *(.text.entry) *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	.text.init : { *(.text.init) }

	. = 0xffffffffffffe000;
	.bss : { *(.bss) }
	kern_tail_addr = .;
}
