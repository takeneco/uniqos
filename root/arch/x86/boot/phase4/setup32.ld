/**
 * @file    arch/x86/boot/phase4/setup32.ld
 * @version 0.0.3
 * @date    2009-07-26
 * @author  Kato.T
 * 
 * phase4 のリンカスクリプト。
 */
/* (C) Kato.T 2009 */

OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS {
	.text : { *(.entry) *(.text) }
	text_end = .;
	.rodata : { *(.rodata) }
	rodata_end = .;
	.data : { *(.data) }
	phase4_end = .;
	/*.bss : { *(.bss) }*/
}
