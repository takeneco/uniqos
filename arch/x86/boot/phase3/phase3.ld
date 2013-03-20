/* FILE : arch/x86/boot/phase3/phase3.ld
 * VER  : 0.0.2
 * LAST : 2009-06-18
 * (C) Kato.T 2009
 *
 * phase3 のリンクスクリプト。
 */

OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS {
	.text : { *(.entry) *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	.bss : { *(.bss) }
	phase3_end = .;
}
