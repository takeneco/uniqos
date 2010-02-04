
OUTPUT_FORMAT(binary)
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS {
	.text : SUBALIGN(1) { *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	.bss : { *(.bss) }
}
