/**
 * @file    arch/x86_64/boot/setup/setup.ld
 * @version 0.0.0.1
 * @author  Kato.T
 * @brief   setup のリンカスクリプト。
 */
/* (C) Kato.T 2010 */

OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS {
	.text : { *(.entry) *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	/*.bss : { *(.bss) }*/
}
