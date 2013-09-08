/*
 * (C) 2011-2013 KATO Takeshi
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
	.data : { *(EXCLUDE_FILE(*/kernel.o */first_process.o) .data) }
	.bss : {
		*(.bss)
		stack_start = .;
		. = . + 0x1000;
		stack_end = .;
	}

	/* TODO:
	 * C++の例外フレームらしい。
	 * clangでは例外を無効化できないため必要。
	 */
	.eh_frame : { *(.eh_frame) }

	kernel = .;
	.kernel : { */kernel.o(.data) }
	kernel_size = . - kernel;

	first_process = .;
	.first_process : { */first_process.o(.data) }
	first_process_size = . - first_process;

	self_size = . - self_baseadr;
}
