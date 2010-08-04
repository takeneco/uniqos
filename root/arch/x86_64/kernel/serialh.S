// @file    arch/x86_64/kernel/serialh.S
// @author  Kato Takeshi
// @brief   Serial device interrupt handler.
//
// (C) 2010 Kato Takeshi.

.section .text

.globl serial_intr_com1_handler
.globl serial_intr_com2_handler

serial_intr_com1_handler:
	call  on_serial_intr_com1
	movb  $0x60 + 4, %al
	outb  %al, $0x20
	iretq

serial_intr_com2_handler:
	call  on_serial_intr_com2
	movb  $0x60 + 3, %al
	outb  %al, $0x20
	iretq
