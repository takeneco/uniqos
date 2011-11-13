/// @brief multiboot entry point
//
// (C) 2011 KATO Takeshi
//

#define ASM_FILE
#include "multiboot2.h"

STACK_SIZE = 0x1000

//
// multiboot header
//

.section .mbh

	.align MULTIBOOT_HEADER_ALIGN

	// magic field
mbh:
	.long MULTIBOOT2_HEADER_MAGIC
	.long MULTIBOOT_ARCHITECTURE_I386
	.long mbh_end - mbh
	.long -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + (mbh_end - mbh))

	// infomation request
inforeq:
	.word 1, 0
	.long inforeq_end - inforeq
	.long 8
	.long 6
inforeq_end:

	// end of tags
	.word 0, 0
	.long 8
mbh_end:


//
// entry point
//

.section .entry

.code32

.globl entry_
entry_:
	cli

	movl %eax, %edx

	movl $(stack_start + STACK_SIZE), %esp

	// clear stack zero.
	xorl %eax, %eax
	movl $stack_start, %edi
	movl $STACK_SIZE / 4, %ecx
	rep stosl

	pushl %ebx  // multiboot infomation
	pushl %edx  // multiboot magic
	call  load
	testl %eax, %eax
	jnz   stop

	lgdtl gdtr

	// Enable PAE (long mode required)
	// Enable PGE
	movl  $0x000000a0, %eax
	movl  %eax, %cr4

	// Set page table
	movl  $load_info, %ebx
	movl  0x8(%ebx), %eax
	movl  %eax, %cr3

	// IA32_EFER.LME = 1
	movl  $0xc0000080, %ecx
	rdmsr
	orw   $0x0100, %ax
	wrmsr

	movl  $0x80000001, %eax
	movl  %eax, %cr0

	pushl $8
	pushl $start64
	lret

stop:
	hlt
	jmp stop

start64:
.code64
	movq  0x0(%ebx), %rax
	jmp   *%rax

.size entry_, .-entry_

	.align 16
gdt:
	// [0x00] null segment
	.word  0, 0, 0, 0

	// [0x08] 64bit code segment
	// base  0x00000000
	// limit 0xffffff
	.word  0xffff, 0x0000, 0x9a00, 0x00af
end_gdt:
gdtr:
	.short end_gdt - gdt - 1
	.long  gdt

	.align 4
	.comm stack_start, STACK_SIZE

