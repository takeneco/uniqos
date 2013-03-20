/**
 * @file    arch/x86_64/boot/bootldr/bootldr.ld
 * @version 0.0.0.1
 * @author  Kato.T
 * @brief   Boot loader linker script.
 */
/* (C) Kato.T 2010 */

OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS {
	.text : { *(.entry) *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	.bss : { *(.bss) }
}
