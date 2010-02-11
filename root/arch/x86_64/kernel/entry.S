/**
 * @file    arch/x86_64/kernel/entry.S
 * @version 0.0.0.1
 * @author  Kato.T
 */
// (C) Kato.T 2010

.section .text.entry

.globl _start

_start:
1:hlt
jmp 1b
