/// @file  interrupt_handler.S
/// @brief interrupt handler.
//
// (C) 2010-2011 KATO Takeshi
//


.macro REG_STORE
	pushq %r11
	pushq %r10
	pushq %r9
	pushq %r8
	pushq %rdi
	pushq %rsi
	pushq %rdx
	pushq %rcx
	pushq %rax
.endm

.macro REG_LOAD
	popq  %rax
	popq  %rcx
	popq  %rdx
	popq  %rsi
	popq  %rdi
	popq  %r8
	popq  %r9
	popq  %r10
	popq  %r11
.endm

.section .text

.globl exception_intr_0_handler
exception_intr_0_handler:
	call  on_exception_intr_0
	iretq

.globl exception_intr_1_handler
exception_intr_1_handler:
	call  on_exception_intr_1
	iretq

.globl exception_intr_2_handler
exception_intr_2_handler:
	call  on_exception_intr_2
	iretq

.globl exception_intr_3_handler
exception_intr_3_handler:
	call  on_exception_intr_3
	iretq

.globl exception_intr_4_handler
exception_intr_4_handler:
	call  on_exception_intr_4
	iretq

.globl exception_intr_5_handler
exception_intr_5_handler:
	call  on_exception_intr_5
	iretq

.globl exception_intr_6_handler
exception_intr_6_handler:
	call  on_exception_intr_6
	iretq

.globl exception_intr_7_handler
exception_intr_7_handler:
	call  on_exception_intr_7
	iretq

.globl exception_intr_8_handler
exception_intr_8_handler:
	call  on_exception_intr_8
	iretq

.globl exception_intr_9_handler
exception_intr_9_handler:
	call  on_exception_intr_9
	iretq

.globl exception_intr_10_handler
exception_intr_10_handler:
	call  on_exception_intr_10
	iretq

.globl exception_intr_11_handler
exception_intr_11_handler:
	call  on_exception_intr_11
	iretq

.globl exception_intr_12_handler
exception_intr_12_handler:
	call  on_exception_intr_12
	iretq

.globl exception_intr_13_handler
exception_intr_13_handler:
	REG_STORE
	call  on_exception_intr_13
	REG_LOAD
	iretq

.globl exception_intr_14_handler
exception_intr_14_handler:
	call  on_exception_intr_14
	iretq

.globl exception_intr_15_handler
exception_intr_15_handler:
	call  on_exception_intr_15
	iretq

.globl exception_intr_16_handler
exception_intr_16_handler:
	call  on_exception_intr_16
	iretq

.globl exception_intr_17_handler
exception_intr_17_handler:
	call  on_exception_intr_17
	iretq

.globl exception_intr_18_handler
exception_intr_18_handler:
	call  on_exception_intr_18
	iretq

.globl exception_intr_19_handler
exception_intr_19_handler:
	call  on_exception_intr_19
	iretq

.globl interrupt_20_handler
interrupt_20_handler:
	REG_STORE
	call  interrupt_timer
	REG_LOAD
	iretq

.macro INTERRUPT_ENTRY method
	REG_STORE
	call \method
	REG_LOAD
	iretq
.endm

store_register:
	movq %rax, 0x08(%rsp)
	movq %rcx, 0x10(%rsp)
	movq %rdx, 0x18(%rsp)
	movq %rsi, 0x20(%rsp)
	movq %rdi, 0x28(%rsp)
	movq %r8,  0x30(%rsp)
	movq %r9,  0x38(%rsp)
	movq %r10, 0x40(%rsp)
	movq %r11, 0x48(%rsp)
	ret

load_register:
	movq 0x08(%rsp), %rax
	movq 0x10(%rsp), %rcx
	movq 0x18(%rsp), %rdx
	movq 0x20(%rsp), %rsi
	movq 0x28(%rsp), %rdi
	movq 0x30(%rsp), %r8
	movq 0x38(%rsp), %r9
	movq 0x40(%rsp), %r10
	movq 0x48(%rsp), %r11
	ret

#define ENTRY_START(symbol) .globl symbol; symbol:
#define ENTRY_END(symbol) .size symbol, .-symbol

.macro interrupt_method vector
	subq $8*9, %rsp
	call store_register
	movl $\vector, %edi
	call on_interrupt
	call load_register
	addq $8*9, %rsp
	iretq
.endm

ENTRY_START(interrupt_5f_handler)
	interrupt_method 0x5f
ENTRY_END(interrupt_5f_handler)

