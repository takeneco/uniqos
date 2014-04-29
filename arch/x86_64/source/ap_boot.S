/// @file  ap_boot.S
/// @brief Start point of application processor.

//  Uniqos  --  Unique Operating System
//  (C) 2012-2013 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

.text

.code16
.globl _start
_start:
	cli

	lgdtl apboot_gdtr
	movl  %cr0, %eax
	andl  $0x1ffaffc0, %eax
	orl   $0x00000001, %eax
	movl  %eax, %cr0
	ljmpl $8*2, $start32

.code32
start32:
	// Enable PAE, PGE
	movl  $0x000000a0, %eax
	movl  %eax, %cr4

	// CANNOT use "movq".
	// Page table must to be locate 32bit address in 32bit mode.
	movl  (param), %edx
	movl  %edx, %cr3

	// IA32_EFER.LME = 1
	movl  $0xc0000080, %ecx
	rdmsr
	orw   $0x0100, %ax
	wrmsr

	movl  %cr0, %ebx
	orl   $0x80040033, %ebx
	movl  %ebx, %cr0

	ljmpl $8*1, $start64

.code64
start64:
	movq  (param+16), %rsp
	jmp   *(param+8)

.align 4
apboot_gdtr:
	.short apboot_gdt_end - apboot_gdt - 1
	.long apboot_gdt

.align 16
apboot_gdt:
	// [0x00] null
	.word 0, 0, 0, 0

	// [0x08] 64bit code segment
	.word 0xffff, 0x0000, 0x9b00, 0x00af

	// [0x10] 32bit code segment
	.word 0xffff, 0x0000, 0x9b00, 0x00cf

	// [0x18] 32bit data segment
	.word 0xffff, 0x0000, 0x9300, 0x00cf
apboot_gdt_end:

.align 8
param:
