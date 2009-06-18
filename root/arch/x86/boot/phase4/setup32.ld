/* FILE : arch/x86/boot/phase4/setup32.ld
 * VAR  : 0.0.2
 * LAST : 2009-06-18
 * (C) Kato.T 2009
 *
 * phase4 のリンカスクリプト。
 */

OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS {
	.text : { *(.entry) *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	.bss : { *(.bss) }
}
