/* @file    arch/x86_64/kernel/pr.lds
 * @author  KATO Takeshi
 * @brief   Linker script of initial process.
 *
 * (C) 2013 KATO Takeshi
 */

OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS {
	. = 0x00100000;
	.text : { *(.text) }
}
