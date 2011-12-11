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

save_regs:
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

load_regs:
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

.macro exception_handler handler func
	ENTRY_START(\handler)
	subq  $8*9, %rsp
	call  save_regs
	call  \func
	call  load_regs
	addq  $8*9, %rsp
	ENTRY_END(\handler)
.endm

exception_handler exception_intr_0x00 on_exception_0x00
exception_handler exception_intr_0x01 on_exception_0x01
exception_handler exception_intr_0x02 on_exception_0x02
exception_handler exception_intr_0x03 on_exception_0x03
exception_handler exception_intr_0x04 on_exception_0x04
exception_handler exception_intr_0x05 on_exception_0x05
exception_handler exception_intr_0x06 on_exception_0x06
exception_handler exception_intr_0x07 on_exception_0x07
exception_handler exception_intr_0x08 on_exception_0x08
exception_handler exception_intr_0x09 on_exception_0x09
exception_handler exception_intr_0x0a on_exception_0x0a
exception_handler exception_intr_0x0b on_exception_0x0b
exception_handler exception_intr_0x0c on_exception_0x0c
exception_handler exception_intr_0x0d on_exception_0x0d
exception_handler exception_intr_0x0e on_exception_0x0e
exception_handler exception_intr_0x0f on_exception_0x0f
exception_handler exception_intr_0x10 on_exception_0x10
exception_handler exception_intr_0x11 on_exception_0x11
exception_handler exception_intr_0x12 on_exception_0x12
exception_handler exception_intr_0x13 on_exception_0x13
exception_handler exception_intr_0x14 on_exception_0x14
exception_handler exception_intr_0x15 on_exception_0x15
exception_handler exception_intr_0x16 on_exception_0x16
exception_handler exception_intr_0x17 on_exception_0x17
exception_handler exception_intr_0x18 on_exception_0x18
exception_handler exception_intr_0x19 on_exception_0x19
exception_handler exception_intr_0x1a on_exception_0x1a
exception_handler exception_intr_0x1b on_exception_0x1b
exception_handler exception_intr_0x1c on_exception_0x1c
exception_handler exception_intr_0x1d on_exception_0x1d
exception_handler exception_intr_0x1e on_exception_0x1e
exception_handler exception_intr_0x1f on_exception_0x1f

.macro intr_handler handler vec
	ENTRY_START(\handler)
	subq $8*9, %rsp
	call save_regs
	movl $\vec, %edi
	call on_interrupt
	call load_regs
	addq $8*9, %rsp
	iretq
	ENTRY_END(\handler)
.endm

intr_handler intr_0x20 0x20
intr_handler intr_0x21 0x21
intr_handler intr_0x22 0x22
intr_handler intr_0x23 0x23
intr_handler intr_0x24 0x24
intr_handler intr_0x25 0x25
intr_handler intr_0x26 0x26
intr_handler intr_0x27 0x27
intr_handler intr_0x28 0x28
intr_handler intr_0x29 0x29
intr_handler intr_0x2a 0x2a
intr_handler intr_0x2b 0x2b
intr_handler intr_0x2c 0x2c
intr_handler intr_0x2d 0x2d
intr_handler intr_0x2e 0x2e
intr_handler intr_0x2f 0x2f

intr_handler intr_0x30 0x30
intr_handler intr_0x31 0x31
intr_handler intr_0x32 0x32
intr_handler intr_0x33 0x33
intr_handler intr_0x34 0x34
intr_handler intr_0x35 0x35
intr_handler intr_0x36 0x36
intr_handler intr_0x37 0x37
intr_handler intr_0x38 0x38
intr_handler intr_0x39 0x39
intr_handler intr_0x3a 0x3a
intr_handler intr_0x3b 0x3b
intr_handler intr_0x3c 0x3c
intr_handler intr_0x3d 0x3d
intr_handler intr_0x3e 0x3e
intr_handler intr_0x3f 0x3f

intr_handler interrupt_0x5e_handler 0x5e
//ENTRY_START(interrupt_0x5e_handler)
//	interrupt_method 0x5e
//ENTRY_END(interrupt_0x5e_handler)

intr_handler interrupt_0x5f_handler 0x5f
//ENTRY_START(interrupt_0x5f_handler)
//	interrupt_method 0x5f
//ENTRY_END(interrupt_0x5f_handler)

