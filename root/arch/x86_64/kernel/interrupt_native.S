/// @file  interrupt_handler.S
/// @brief interrupt handler.
//
// (C) 2010-2012 KATO Takeshi
//


.section .text

save_regs2:
	movq %rax, 0x08(%rsp)
	movq %rcx, 0x10(%rsp)
	movq %rdx, 0x18(%rsp)
	movq %rsi, 0x20(%rsp)
	movq %rdi, 0x28(%rsp)
	movq %r8,  0x30(%rsp)
	movq %r9,  0x38(%rsp)
	movq %r10, 0x40(%rsp)
	movq %r11, 0x48(%rsp)

	movq %rbx, 0x50(%rsp)
	movq %rbp, 0x58(%rsp)
	movq %r12, 0x60(%rsp)
	movq %r13, 0x68(%rsp)
	movq %r14, 0x70(%rsp)
	movq %r15, 0x78(%rsp)
	ret

load_regs2:
	movq 0x08(%rsp), %rax
	movq 0x10(%rsp), %rcx
	movq 0x18(%rsp), %rdx
	movq 0x20(%rsp), %rsi
	movq 0x28(%rsp), %rdi
	movq 0x30(%rsp), %r8
	movq 0x38(%rsp), %r9
	movq 0x40(%rsp), %r10
	movq 0x48(%rsp), %r11

	movq 0x50(%rsp), %rbx
	movq 0x58(%rsp), %rbp
	movq 0x60(%rsp), %r12
	movq 0x68(%rsp), %r13
	movq 0x70(%rsp), %r14
	movq 0x78(%rsp), %r15
	ret

#define ENTRY_START(symbol) .globl symbol; symbol:
#define ENTRY_END(symbol) .size symbol, .-symbol

.macro exception_handler handler, func
	ENTRY_START(\handler)
	subq  $8 * 15, %rsp
	call  save_regs2
	call  \func
	call  load_regs2
	addq  $8 * 15 + 8, %rsp
	iretq
	ENTRY_END(\handler)
.endm

exception_handler exception_intr_0x00, on_exception_0x00
exception_handler exception_intr_0x01, on_exception_0x01
exception_handler exception_intr_0x02, on_exception_0x02
exception_handler exception_intr_0x03, on_exception_0x03
exception_handler exception_intr_0x04, on_exception_0x04
exception_handler exception_intr_0x05, on_exception_0x05
exception_handler exception_intr_0x06, on_exception_0x06
exception_handler exception_intr_0x07, on_exception_0x07
exception_handler exception_intr_0x08, on_exception_0x08
exception_handler exception_intr_0x09, on_exception_0x09
exception_handler exception_intr_0x0a, on_exception_0x0a
exception_handler exception_intr_0x0b, on_exception_0x0b
exception_handler exception_intr_0x0c, on_exception_0x0c
exception_handler exception_intr_0x0d, on_exception_0x0d
exception_handler exception_intr_0x0e, on_exception_0x0e
exception_handler exception_intr_0x0f, on_exception_0x0f
exception_handler exception_intr_0x10, on_exception_0x10
exception_handler exception_intr_0x11, on_exception_0x11
exception_handler exception_intr_0x12, on_exception_0x12
exception_handler exception_intr_0x13, on_exception_0x13
exception_handler exception_intr_0x14, on_exception_0x14
exception_handler exception_intr_0x15, on_exception_0x15
exception_handler exception_intr_0x16, on_exception_0x16
exception_handler exception_intr_0x17, on_exception_0x17
exception_handler exception_intr_0x18, on_exception_0x18
exception_handler exception_intr_0x19, on_exception_0x19
exception_handler exception_intr_0x1a, on_exception_0x1a
exception_handler exception_intr_0x1b, on_exception_0x1b
exception_handler exception_intr_0x1c, on_exception_0x1c
exception_handler exception_intr_0x1d, on_exception_0x1d
exception_handler exception_intr_0x1e, on_exception_0x1e
exception_handler exception_intr_0x1f, on_exception_0x1f

.macro intr_handler2 handler, vec
	ENTRY_START(\handler)
	subq $8 * 15, %rsp
	call save_regs2
	movl $\vec, %edi
	call on_interrupt
	call load_regs2
	addq $8 * 15, %rsp
	iretq
	ENTRY_END(\handler)
.endm


// Context switch
//
// input param:
//   %rdi is pointer to new thread regset
//   %rsi is pointer to old thread regset
// input stack layout:
//   (%rsp)  return address
//
// save regs:
//   %rbx, %rbp, %r12, %r13, %r14, %r15, %rf,
//   %rip, %cs, %ds, %es, %rsp, %ss, %fs, %gs
// load regs:
//   all
ENTRY_START(switch_regset)
	// save regs
	popq  %rax                // load %rip (return address)
	pushf
	popq  %rcx                // load %rf
	movq  %rbx, 8* 3(%rsi)
	movq  %rbp, 8* 4(%rsi)
	movq  %r12, 8*11(%rsi)
	movq  %r13, 8*12(%rsi)
	movq  %r14, 8*13(%rsi)
	movq  %r15, 8*14(%rsi)
	movq  %rax, 8*15(%rsi)    // save %rip
	movw  %cs, %ax
	movw  %ds, %dx
	movw  %es, %bx
	movw  %ax,  8*16  (%rsi)  // save %cs
	movw  %dx,  8*16+2(%rsi)  // save %ds
	movw  %bx,  8*16+4(%rsi)  // save %es
	movq  %rcx, 8*17(%rsi)    // save %rf
	movq  %rsp, 8*18(%rsi)
	movw  %ss, %ax
	movw  %fs, %dx
	movw  %gs, %bx
	movw  %ax,  8*19  (%rsi)  // save %ss
	movw  %dx,  8*19+2(%rsi)  // save %fs
	movw  %bx,  8*19+2(%rsi)  // save %gs

        // load regs
	jmp   load_regs
ENTRY_END(switch_regset)


// Save regs to regset
//
// input stack layout:
//      (%rsp)  return address
//   8*1(%rsp)  error code or 0
//   8*2(%rsp)  saved %rip
//   8*3(%rsp)  saved %cs
//   8*4(%rsp)  saved %rf
//   8*5(%rsp)  saved %rsp
//   8*6(%rsp)  saved %ss
//   8*7(%rsp)  ptr to ptr to regset
//   8*8(%rsp)  ptr to cpu_ctl::thread
save_regs_on_intr:
	movq  %rdi, -8(%rsp)
	movq  8*7(%rsp), %rdi
	movq  (%rdi), %rdi        // load regset

	movq  %rax,     (%rdi)
	movq  %rcx, 8* 1(%rdi)
	movq  %rdx, 8* 2(%rdi)
	movq  %rbx, 8* 3(%rdi)
	movq  %rbp, 8* 4(%rdi)
	movq  %rsi, 8* 5(%rdi)
	movq   -8(%rsp), %rax     // load %rdi
	movq  8*2(%rsp), %rbx     // load %rip
	movw  8*3(%rsp), %cx      // load %cs
	movw  %ds, %dx
	movw  %es, %bp
	movq  8*4(%rsp), %rsi     // load %rf
	movq  %rax, 8* 6(%rdi)    // save %rdi
	movq  %r8,  8* 7(%rdi)
	movq  %r9,  8* 8(%rdi)
	movq  %r10, 8* 9(%rdi)
	movq  %r11, 8*10(%rdi)
	movq  %r12, 8*11(%rdi)
	movq  %r13, 8*12(%rdi)
	movq  %r14, 8*13(%rdi)
	movq  %r15, 8*14(%rdi)
	movq  %rbx, 8*15(%rdi)    // save %rip
	movw  %cx,  8*16  (%rdi)  // save %cs
	movw  %dx,  8*16+2(%rdi)  // save %ds
	movw  %bp,  8*16+4(%rdi)  // save %es
	movq  %rsi, 8*17(%rdi)    // save %rf
	movq  8*5(%rsp), %rax     // load %rsp
	movw  8*6(%rsp), %bx      // load %ss
	movw  %fs, %cx
	movw  %gs, %dx
	movq  %rax, 8*18  (%rdi)  // save %rsp
	movw  %bx,  8*19  (%rdi)  // save %ss
	movw  %cx,  8*19+2(%rdi)  // save %fs
	movw  %dx,  8*19+4(%rdi)  // save %gs
	ret


// Load regs and jump to new thread
//
// input stack layout:
//      (%rsp)  error code or 0
//   8*1(%rsp)  old data
//   8*2(%rsp)  old data
//   8*3(%rsp)  old data
//   8*4(%rsp)  old data
//   8*5(%rsp)  old data
//   8*6(%rsp)  ptr to ptr to new regset
//   8*7(%rsp)  ptr to new cpu_ctl::thread
load_regs_on_intr:
	movq  8*6(%rsp), %rdi
	movq  (%rdi), %rdi        // load regset

// input:
//   %rdi is ptr to regset
load_regs:
	movw  8*19+4 (%rdi), %dx  // load %gs
	movw  8*19+2 (%rdi), %cx  // load %fs
	movw  %dx, %gs
	movw  %cx, %fs
	movw  8*16+4 (%rdi), %bp  // load %es
	movw  8*16+2 (%rdi), %dx  // load %ds
	movq  8*14(%rdi), %r15
	movq  8*13(%rdi), %r14
	movq  8*12(%rdi), %r13
	movq  8*11(%rdi), %r12
	movq  8*10(%rdi), %r11
	movq  8* 9(%rdi), %r10
	movq  8* 8(%rdi), %r9
	movq  8* 7(%rdi), %r8
	movw  %bp, %es
	movw  %dx, %ds
	movq  8* 5(%rdi), %rsi
	movq  8* 4(%rdi), %rbp
	movq  8* 3(%rdi), %rbx
	movq  8* 2(%rdi), %rdx
	movq  8* 1(%rdi), %rcx
	movq      (%rdi), %rax

	leaq  8*15(%rdi), %rsp
        movq  8*-9(%rsp), %rdi    // load %rdi
	iretq


// Interrupt handler
//
// stack layout :
//      (%rsp)  saved %rip
//   8*1(%rsp)  saved %cs
//   8*2(%rsp)  saved %rf
//   8*3(%rsp)  saved %rsp
//   8*4(%rsp)  saved %ss
//   8*5(%rsp)  ptr to ptr to regset
//   8*6(%rsp)  ptr to cpu_ctl::thread
.macro intr_handler handler, vec
	ENTRY_START(\handler)
	pushq $0
	call  save_regs_on_intr
	movl  $\vec, %edi
	call  on_interrupt
	jmp   load_regs_on_intr
	ENTRY_END(\handler)
.endm

intr_handler intr_0x20, 0x20
intr_handler intr_0x21, 0x21
intr_handler intr_0x22, 0x22
intr_handler intr_0x23, 0x23
intr_handler intr_0x24, 0x24
intr_handler intr_0x25, 0x25
intr_handler intr_0x26, 0x26
intr_handler intr_0x27, 0x27
intr_handler intr_0x28, 0x28
intr_handler intr_0x29, 0x29
intr_handler intr_0x2a, 0x2a
intr_handler intr_0x2b, 0x2b
intr_handler intr_0x2c, 0x2c
intr_handler intr_0x2d, 0x2d
intr_handler intr_0x2e, 0x2e
intr_handler intr_0x2f, 0x2f

intr_handler intr_0x30, 0x30
intr_handler intr_0x31, 0x31
intr_handler intr_0x32, 0x32
intr_handler intr_0x33, 0x33
intr_handler intr_0x34, 0x34
intr_handler intr_0x35, 0x35
intr_handler intr_0x36, 0x36
intr_handler intr_0x37, 0x37
intr_handler intr_0x38, 0x38
intr_handler intr_0x39, 0x39
intr_handler intr_0x3a, 0x3a
intr_handler intr_0x3b, 0x3b
intr_handler intr_0x3c, 0x3c
intr_handler intr_0x3d, 0x3d
intr_handler intr_0x3e, 0x3e
intr_handler intr_0x3f, 0x3f

intr_handler intr_0x40, 0x40
intr_handler intr_0x41, 0x41
intr_handler intr_0x42, 0x42
intr_handler intr_0x43, 0x43
intr_handler intr_0x44, 0x44
intr_handler intr_0x45, 0x45
intr_handler intr_0x46, 0x46
intr_handler intr_0x47, 0x47
intr_handler intr_0x48, 0x48
intr_handler intr_0x49, 0x49
intr_handler intr_0x4a, 0x4a
intr_handler intr_0x4b, 0x4b
intr_handler intr_0x4c, 0x4c
intr_handler intr_0x4d, 0x4d
intr_handler intr_0x4e, 0x4e
intr_handler intr_0x4f, 0x4f

intr_handler intr_0x50, 0x50
intr_handler intr_0x51, 0x51
intr_handler intr_0x52, 0x52
intr_handler intr_0x53, 0x53
intr_handler intr_0x54, 0x54
intr_handler intr_0x55, 0x55
intr_handler intr_0x56, 0x56
intr_handler intr_0x57, 0x57
intr_handler intr_0x58, 0x58
intr_handler intr_0x59, 0x59
intr_handler intr_0x5a, 0x5a
intr_handler intr_0x5b, 0x5b
intr_handler intr_0x5c, 0x5c
intr_handler intr_0x5d, 0x5d
intr_handler intr_0x5e, 0x5e
intr_handler intr_0x5f, 0x5f

intr_handler intr_0x60, 0x60
intr_handler intr_0x61, 0x61
intr_handler intr_0x62, 0x62
intr_handler intr_0x63, 0x63
intr_handler intr_0x64, 0x64
intr_handler intr_0x65, 0x65
intr_handler intr_0x66, 0x66
intr_handler intr_0x67, 0x67
intr_handler intr_0x68, 0x68
intr_handler intr_0x69, 0x69
intr_handler intr_0x6a, 0x6a
intr_handler intr_0x6b, 0x6b
intr_handler intr_0x6c, 0x6c
intr_handler intr_0x6d, 0x6d
intr_handler intr_0x6e, 0x6e
intr_handler intr_0x6f, 0x6f

intr_handler intr_0x70, 0x70
intr_handler intr_0x71, 0x71
intr_handler intr_0x72, 0x72
intr_handler intr_0x73, 0x73
intr_handler intr_0x74, 0x74
intr_handler intr_0x75, 0x75
intr_handler intr_0x76, 0x76
intr_handler intr_0x77, 0x77
intr_handler intr_0x78, 0x78
intr_handler intr_0x79, 0x79
intr_handler intr_0x7a, 0x7a
intr_handler intr_0x7b, 0x7b
intr_handler intr_0x7c, 0x7c
intr_handler intr_0x7d, 0x5d
intr_handler intr_0x7e, 0x7e
intr_handler intr_0x7f, 0x7f

intr_handler intr_0x80, 0x80
intr_handler intr_0x81, 0x81
intr_handler intr_0x82, 0x82
intr_handler intr_0x83, 0x83
intr_handler intr_0x84, 0x84
intr_handler intr_0x85, 0x85
intr_handler intr_0x86, 0x86
intr_handler intr_0x87, 0x87
intr_handler intr_0x88, 0x88
intr_handler intr_0x89, 0x89
intr_handler intr_0x8a, 0x8a
intr_handler intr_0x8b, 0x8b
intr_handler intr_0x8c, 0x8c
intr_handler intr_0x8d, 0x8d
intr_handler intr_0x8e, 0x8e
intr_handler intr_0x8f, 0x8f

intr_handler interrupt_0x5e_handler, 0x5e
//ENTRY_START(interrupt_0x5e_handler)
//	interrupt_method 0x5e
//ENTRY_END(interrupt_0x5e_handler)

intr_handler interrupt_0x5f_handler, 0x5f
//ENTRY_START(interrupt_0x5f_handler)
//	interrupt_method 0x5f
//ENTRY_END(interrupt_0x5f_handler)

retf
