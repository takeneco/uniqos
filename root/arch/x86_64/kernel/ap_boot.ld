/* @file ap_boot.ld
 * (C) 2012 KATO Takeshi
 */

OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS {
	. = 0x1000;
	.text : { *(.text) }
}
