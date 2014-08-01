/// @file  syscall.S
/// @brief syscall handler.
//
// (C) 2014 KATO Takeshi
//


.section .text


#define ENTRY_START(symbol) .globl symbol; symbol:
#define ENTRY_END(symbol)   .size symbol, .-symbol
#define LOCAL_START(symbol) symbol:
#define LOCAL_END(symbol)   .size symbol, .-symbol

// syscall1(uptr _1st, syscall_id)
ENTRY_START(syscall1)
	// rdi -> rdi
	// rsi -> rax

	movq  %rsi, %rax
	syscall
	ret

ENTRY_END(syscall1)

// syscall2(uptr _1st, uptr _2nd, syscall_id)
ENTRY_START(syscall2)
	// rdi -> rdi
	// rsi -> rsi
	// rdx -> rax

	movq  %rdx, %rax
	syscall
	ret

ENTRY_END(syscall2)

// syscall3(_1st, _2nd, _3rd, syscall_id)
ENTRY_START(syscall3)
	// rdi -> rdi
	// rsi -> rsi
	// rdx -> rdx
	// rcx -> rax

	movq  %rcx, %rax
	syscall
	ret

ENTRY_END(syscall3)

// syscall4
ENTRY_START(syscall4)
	// rdi -> rdi
	// rsi -> rsi
	// rdx -> rdx
	// rcx -> r10
	// r8  -> rax

	movq  %rcx, %r10
	movq  %r8, %rax
	syscall
	ret

ENTRY_END(syscall4)

// syscall5
ENTRY_START(syscall5)
	// rdi -> rdi
	// rsi -> rsi
	// rdx -> rdx
	// rcx -> r10
	// r8  -> r8
	// r9  -> rax

	movq  %rcx, %r10
	movq  %r9, %rax
	syscall
	ret

ENTRY_END(syscall5)

// syscall6
ENTRY_START(syscall6)
	// rdi -> rdi
	// rsi -> rsi
	// rdx -> rdx
	// rcx -> r10
	// r8  -> r8
	// r9  -> r9
	// pop rax

	movq  %rcx, %r10
	movq  8(%rsp), %rax
	syscall
	ret

ENTRY_END(syscall6)

