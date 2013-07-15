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
	/*
	cli
	swapgs

	movq  %rdi, %gs:8
	movq  %gs:0, %rdi
	movq  (%rdi), %rdi

	movq  %rax,     (%rdi)
	movq  %rcx, 8* 1(%rdi)
	movq  %rdx, 8* 2(%rdi)
	movq  %rbx, 8* 3(%rdi)
	movq  %rbp, 8* 4(%rdi)
	movq  %rsi, 8* 5(%rdi)
	movq  %r8,  8* 7(%rdi)
	movq  %r9,  8* 8(%rdi)
	movq  %r10, 8* 9(%rdi)
	movq  %r11, 8*10(%rdi)
	movq  %r12, 8*11(%rdi)
	movq  %r13, 8*12(%rdi)
	movq  %r14, 8*13(%rdi)
	movq  %r15, 8*14(%rdi)
	movq  %rcx, 8*15(%rdi)  // %rip
	movq  %r11, 8*17(%rdi)  // %rf
	movq  %gs:8, %rdi

	swapgs
	sti

	movq  8*17(%rdi), %rcx  // %rf
	movq  8*15(%rdi), %r11
	*/
	sysretq

ENTRY_END(on_syscall)

