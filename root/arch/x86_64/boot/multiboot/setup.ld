/*
 * (C) 2011 KATO Takeshi
 */

OUTPUT_FORMAT(elf64-x86-64)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS {
	/* page size + elf header size */
	. = 0x200000 + SIZEOF_HEADERS;
	.text : { *(.entry) *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	.bss : { *(.bss) }

	/* C++の例外フレームらしい。
	 * clangでは例外を無効化できないため必要。
	 */
	.eh_frame : { *(.eh_frame) }
}
