/// @file  interrupt_handler.S
/// @brief interrupt handler.
//
// (C) 2010-2011 KATO Takeshi
//

.section .text

.globl interrupt_timer_handler
interrupt_timer_handler:
	call  on_interrupt_timer
	iretq
