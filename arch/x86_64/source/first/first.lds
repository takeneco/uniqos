/**
 * @file    first.lds
 * @brief   Linker script of first process.
 */
/*
 * (C) 2013-2014 KATO Takeshi
 */

OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS {
	. = 0x00100000;
	.text : { *(.start) *(.text) }
}
