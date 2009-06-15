/* FILE : arch/x86/boot/phase2/phase2.S
 * VER  : 0.0.3
 * LAST : 2009-05-24
 * (C) Kato.T 2008-2009
 *
 * phase2 のエントリーポイント。
 * phase2 はブートセクタの次の処理。
 * カーネルを読み込み、処理をカーネルに移す。
 */

#include "boot.h"

.code16gcc

.section .text

.global _start

_start:
	// loadkern.cpp を呼び出す。
	call  load_kernel

	/*
	 * loadkern.cpp は phase3 を PHASE3_SEG:0 に読み込むので、
	 * 帰ってきたらそこにジャンプする。
	 * loadkern.cpp はカーネルのサイズを返すので、
	 * それをパラメータとして %ecx 経由でカーネルに渡す。
	 */
	movl  %eax, %ecx
	jmp   $PHASE3_SEG, $0

/*
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
*/
