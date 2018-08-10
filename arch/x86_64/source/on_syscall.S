/// @file  on_syscall.S
/// @brief syscall handler.

//  Uniqos  --  Unique Operating System
//  (C) 2013 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#define ENTRY_START(symbol) .globl symbol; symbol:
#define ENTRY_END(symbol)   .size symbol, .-symbol


.section .text

// syscall
ENTRY_START(on_syscall)
    swapgs
    movq  %rsp, %gs:8
    movq  %gs:0, %rsp

    // callee saved: %rbx, %rbp, %r12, %r13, %r14, %r15

    pushq %r11   // %eflags
    pushq %rcx   // %rip
    pushq %r9    // 6th param
    pushq %r8    // 5th param
    pushq %r10   // 4th param
    pushq %rdx   // 3rd param
    pushq %rsi   // 2nd param
    pushq %rdi   // 1st param
    pushq %rax   // system call num
    movq  %gs:8, %rsi
    movq  %rsi, 8*9(%rsp)
    swapgs

    movq  %rsp, %rdi
    call  syscall_entry

    addq  $8*7, %rsp
    popq  %rcx
    popq  %r11
    popq  %rsp

    sysretq

ENTRY_END(on_syscall)

