/// @file  task_switch.S
/// @brief Task switching.
//
// (C) 2010 KATO Takeshi
//


.section .text

.globl task_switch
task_switch:
	// store thread_state of previous task
	// (rax) is thread_state
	movq   %rbx,    (%rax)
	movq   %rcx,   8(%rax)
	movq   %rdx,  16(%rax)
	movq   %rbp,  24(%rax)
	movq   %rsi,  32(%rax)
	movq   %rdi,  40(%rax)
	movq   %r8,   48(%rax)
	movq   %r9,   56(%rax)
	movq   %r10,  64(%rax)
	movq   %r11,  72(%rax)
	movq   %r12,  80(%rax)
	movq   %r13,  88(%rax)
	movq   %r14,  96(%rax)
	movq   %r15, 104(%rax)
	movq   %rsp, 112(%rax)
	pushfq
	popq   %rdx
	movq   %rdx, 120(%rax)

	// load thread_state of next task
	movq  %rcx, %rax  // (rcx) is thread_state
	movq     (%rax), %rbx
	movq    8(%rax), %rcx
	movq   16(%rax), %rdx
	movq   24(%rax), %rbp
	movq   32(%rax), %rsi
	movq   40(%rax), %rdi
	movq   48(%rax), %r8
	movq   56(%rax), %r9
	movq   64(%rax), %r10
	movq   72(%rax), %r11
	movq   80(%rax), %r12
	movq   88(%rax), %r13
	movq   96(%rax), %r14
	movq  104(%rax), %r15
	movq  112(%rax), %rsp
	movq  120(%rax), %rdx
	pushq %rdx
	popfq

	retq
