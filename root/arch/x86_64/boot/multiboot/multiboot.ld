/*
 * (C) 2011 KATO Takeshi
 */

OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(entry_)

SECTIONS {
	PROVIDE(__gxx_personality_v0 = .);

	/* page size + elf header size */
	. = 0x200000 + SIZEOF_HEADERS;
	.text : { *(.mbh) *(.entry) *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	.bss : { *(.bss) }

	/* C++の例外フレームらしい。
	 * clangでは例外を無効化できないため必要。
	 */
	.eh_frame : { *(.eh_frame) }
}
