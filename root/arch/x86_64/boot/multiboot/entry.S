
#define ASM_FILE
#include "multiboot2.h"

.section .text

.code32

.globl _start
_start:
	jmp entry
	.align 8

multiboot_header:
	.long MULTIBOOT2_HEADER_MAGIC
	.long MULTIBOOT_ARCHITECTURE_I386
	.long multiboot_header_end - multiboot_header
	.long -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + (multiboot_header_end - multiboot_header))
multiboot_header_end:

entry:
	hlt
	jmp entry

.size _start, .-_start
