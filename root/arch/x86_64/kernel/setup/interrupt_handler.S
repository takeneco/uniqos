/// @file  interrupt_handler.S
/// @brief interrupt handler.
//
// (C) 2010-2011 KATO Takeshi
//

.section .text

.globl interrupt_trap_handler
interrupt_trap_handler:
	iretq

.globl interrupt_intr_handler
interrupt_intr_handler:
	pushq %rbx
	pushq %rbp
	pushq %rsi
	pushq %rdi
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15

	pushq %rax
	pushq %rcx
	pushq %rdx
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
	call  on_interrupt
	popq  %r11
	popq  %r10
	popq  %r9
	popq  %r8
	popq  %rdx
	popq  %rcx
	popq  %rax

	popq  %r15
	popq  %r14
	popq  %r13
	popq  %r12
	popq  %rdi
	popq  %rsi
	popq  %rbp
	popq  %rbx

	iretq
