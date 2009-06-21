/* FILE : arch/x86/kernel/kernel.ld
 * VAR  : 0.0.1
 * LAST : 2009-06-21
 * (C) Kato.T 2009
 *
 * カーネル本体のリンカスクリプト。
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
