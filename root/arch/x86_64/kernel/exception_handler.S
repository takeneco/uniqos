/// @file  exception_handler.S
/// @brief Exception interrupt handler.
//
// (C) 2010 KATO Takeshi
//

.section .text

.globl exception_intr_0_handler
exception_intr_0_handler:
	call  on_exception_intr_0
	iret

.globl exception_intr_1_handler
exception_intr_1_handler:
	call  on_exception_intr_1
	iret

.globl exception_intr_2_handler
exception_intr_2_handler:
	call  on_exception_intr_2
	iret

.globl exception_intr_3_handler
exception_intr_3_handler:
	call  on_exception_intr_3
	iret

.globl exception_intr_4_handler
exception_intr_4_handler:
	call  on_exception_intr_4
	iret

.globl exception_intr_5_handler
exception_intr_5_handler:
	call  on_exception_intr_5
	iret

.globl exception_intr_6_handler
exception_intr_6_handler:
	call  on_exception_intr_6
	iret

.globl exception_intr_7_handler
exception_intr_7_handler:
	call  on_exception_intr_7
	iret

.globl exception_intr_8_handler
exception_intr_8_handler:
	call  on_exception_intr_8
	iret

.globl exception_intr_9_handler
exception_intr_9_handler:
	call  on_exception_intr_9
	iret

.globl exception_intr_10_handler
exception_intr_10_handler:
	call  on_exception_intr_10
	iret

.globl exception_intr_11_handler
exception_intr_11_handler:
	call  on_exception_intr_11
	iret

.globl exception_intr_12_handler
exception_intr_12_handler:
	call  on_exception_intr_12
	iret

.globl exception_intr_13_handler
exception_intr_13_handler:
	call  on_exception_intr_13
	iret

.globl exception_intr_14_handler
exception_intr_14_handler:
	call  on_exception_intr_14
	iret

.globl exception_intr_15_handler
exception_intr_15_handler:
	call  on_exception_intr_15
	iret

.globl exception_intr_16_handler
exception_intr_16_handler:
	call  on_exception_intr_16
	iret

.globl exception_intr_17_handler
exception_intr_17_handler:
	call  on_exception_intr_17
	iret

.globl exception_intr_18_handler
exception_intr_18_handler:
	call  on_exception_intr_18
	iret

.globl exception_intr_19_handler
exception_intr_19_handler:
	call  on_exception_intr_19
	iret

.globl intr_30_handler
intr_30_handler:
	call  on_timer
	call  on_timer
	iretq

