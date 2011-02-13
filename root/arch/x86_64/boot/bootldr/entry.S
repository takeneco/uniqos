/// @file  bootldr.S
/// @brief Boot loader.
//
// (C) 2010 KATO Takeshi
//

#include "boot.h"

.code16gcc

.section .entry

.global _start
_start:
	// スタックは bootsect のスタックをそのまま使う。

	movw  %cs, %ax
	movw  %ax, %ds
	movw  %ax, %es

	// enable a20 gate
        movw  $0x2401, %ax
        int   $0x15
	//pushl %eax

	call  wait_kbc
	movb  $0xd1, %al
	outb  %al, $0x64
	call  wait_kbc
	movb  $0xdf, %al
	outb  %al, $0x60
	call  wait_kbc

	// loadkern.cpp を呼び出す。
	call  load_kernel

	/*
	 * loadkern.cpp は setup(kernel head) を SETUP_SEG:SETUP_ADR に
	 * 読み込むので、帰ってきたらそこにジャンプする。
	 * loadkern.cpp はカーネルのサイズを返すので、
	 * それをパラメータとして %ecx 経由でカーネルに渡す。
	 */
	movl  %eax, %ecx
	jmp   $SETUP_SEG, $SETUP_ADR

wait_kbc:
	inb   $0x64, %al
	andb  $0x02, %al
	inb   $0x60, %al
	// andb の結果が 0 ならループ終了する。
	jnz   wait_kbc
	ret

// 文字列を表示する
// %si : 表示する文字列のポインタ
// %si, %ax, %bx は破壊される
display:
	lodsb
	testb %al, %al
	jz    1f
	movb  $0x0e, %ah
	movb  $15, %bh  // 白
	int   $0x10
	jmp   display
1:
	ret
