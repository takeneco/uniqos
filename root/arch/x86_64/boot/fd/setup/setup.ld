/**
 * @file    arch/x86_64/boot/setup/setup.ld
 * @author  Kato Takeshi
 * @brief   setup のリンカスクリプト。
 *
 * (C) Kato Takeshi 2010
 */

OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS {
	. = 0x500;
	setup_body_start = .;
	.text : { *(.entry) *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	/*.bss : { *(.bss) }*/

	/* C++の例外フレームらしい。
	 * clangでは例外を無効化できないため必要。
	 */
	.eh_frame : { *(.eh_frame) }

	kern_body_start = .;
}
