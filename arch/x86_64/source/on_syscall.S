/// @file  on_syscall.S
/// @brief syscall handler.
//
// (C) 2013 KATO Takeshi
//


.section .text


#define ENTRY_START(symbol) .globl symbol; symbol:
#define ENTRY_END(symbol)   .size symbol, .-symbol
#define LOCAL_START(symbol) symbol:
#define LOCAL_END(symbol)   .size symbol, .-symbol

// syscall
ENTRY_START(on_syscall)
	swapgs
	movq  %rsp, %gs:8
	movq  %gs:0, %rsp
	//pushq %r15 // C saved
	//pushq %r14 // C saved
	//pushq %r13 // C saved
	//pushq %r12 // C saved
	pushq %r11   // %eflags
	pushq %r10   // 4th param
	pushq %r9    // 6th param
	pushq %r8    // 5th param
	pushq %rdi   // 1st param
	pushq %rsi   // 2nd param
	//pushq %rbp // C saved
	pushq %rdx   // 3rd param
	pushq %rcx   // %rip
	//pushq %rbx // C saved
	pushq %rax   // system call num
	movq  %gs:8, %rsi
	movq  %rsi, 72(%rsp)
	swapgs

	movq  %rsp, %rdi
	call  syscall_entry

	movq  8(%rsp), %rcx
	addq  $8*3, %rsp
	popq  %rsi
	popq  %rdi
	popq  %r8
	popq  %r9
	popq  %r10
	popq  %r11
	popq  %rsp

	sysretq

ENTRY_END(on_syscall)

