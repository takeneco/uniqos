/*
 * (C) 2011 KATO Takeshi
 */

OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(entry_)

SECTIONS {
	PROVIDE(__gxx_personality_v0 = .);

	self_baseadr = 0x200000;
	/* page size + elf header size */
	. = self_baseadr + SIZEOF_HEADERS;
	.text : { *(.mbh) *(.entry) *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(EXCLUDE_FILE(*/kernel.o) .data) }
	.bss : { *(.bss) }

	/* C++の例外フレームらしい。
	 * clangでは例外を無効化できないため必要。
	 */
	.eh_frame : { *(.eh_frame) }

	kernel = .;
	.kernel : { */kernel.o(.data) }
	kernel_size = . - kernel;
	self_size = . - self_baseadr;
}
