/// @file  interrupt_handler.S
/// @brief interrupt handler.
//
// (C) 2010-2011 KATO Takeshi
//

.section .text

.globl exception_intr_0_handler
exception_intr_0_handler:
	call  on_exception_intr_0
	iretq

.globl exception_intr_1_handler
exception_intr_1_handler:
	call  on_exception_intr_1
	iretq

.globl exception_intr_2_handler
exception_intr_2_handler:
	call  on_exception_intr_2
	iretq

.globl exception_intr_3_handler
exception_intr_3_handler:
	call  on_exception_intr_3
	iretq

.globl exception_intr_4_handler
exception_intr_4_handler:
	call  on_exception_intr_4
	iretq

.globl exception_intr_5_handler
exception_intr_5_handler:
	call  on_exception_intr_5
	iretq

.globl exception_intr_6_handler
exception_intr_6_handler:
	call  on_exception_intr_6
	iretq

.globl exception_intr_7_handler
exception_intr_7_handler:
	call  on_exception_intr_7
	iretq

.globl exception_intr_8_handler
exception_intr_8_handler:
	call  on_exception_intr_8
	iretq

.globl exception_intr_9_handler
exception_intr_9_handler:
	call  on_exception_intr_9
	iretq

.globl exception_intr_10_handler
exception_intr_10_handler:
	call  on_exception_intr_10
	iretq

.globl exception_intr_11_handler
exception_intr_11_handler:
	call  on_exception_intr_11
	iretq

.globl exception_intr_12_handler
exception_intr_12_handler:
	call  on_exception_intr_12
	iretq

.globl exception_intr_13_handler
exception_intr_13_handler:
	call  on_exception_intr_13
	iretq

.globl exception_intr_14_handler
exception_intr_14_handler:
	call  on_exception_intr_14
	iretq

.globl exception_intr_15_handler
exception_intr_15_handler:
	call  on_exception_intr_15
	iretq

.globl exception_intr_16_handler
exception_intr_16_handler:
	call  on_exception_intr_16
	iretq

.globl exception_intr_17_handler
exception_intr_17_handler:
	call  on_exception_intr_17
	iretq

.globl exception_intr_18_handler
exception_intr_18_handler:
	call  on_exception_intr_18
	iretq

.globl exception_intr_19_handler
exception_intr_19_handler:
	call  on_exception_intr_19
	iretq

.globl interrupt_20_handler
interrupt_20_handler:
	call  interrupt_timer
	iretq

