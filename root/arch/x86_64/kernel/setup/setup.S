/**
 * @file   setup.S
 * @brief  カーネル自身のセットアップ。
 *
 * - 16bit real mode から始まり、32bit protect mode を経由して
 *   64bit long mode へ移行する。
 * - Create minimum paging table, and jump kernel body.
 *
 * Bootloader から渡されるパラメータ
 * - %ecx : カーネルのサイズ
 */

/* (C) 2009-2011 KATO Takeshi */

#include "setup.h"

PAGE_SIZE = 4096

.section .entry

.globl _start
.globl a20isopen, a20isclose

_start:
.code16
	pushw %cs
	popw  %ds
	movw  $SETUP_STACK_ADR >> 4, %ax
	movw  %ax, %ss
	xorw  %ax, %ax
	movw  %ax, %sp

	// Kernel size
	pushl %ecx

	movw  $setup_msg, %si
	call  display16

	// Protect I/O interrupt
	movb  $0xff, %al
	outb  %al, $0x21
	nop
	outb  %al, $0xa1
	cli

	// A20 はブートローダが開くので本当はここは不要。
	call  a20_test_bios
	movw  %ax, %si
	call  display16

	call  a20_test_mem
	movw  %ax, %si
	call  display16

	call  a20_open_bios  // A20 open

	call  a20_test_bios
	movw  %ax, %si
	call  display16

	call  a20_test_mem
	movw  %ax, %si
	call  display16
	// ここまで A20

	// 画面モード設定
	// AH = 0x00
	// AL = 0x03 : 16色テキスト 80x25
	//      0x12 : 640x480x4bits
	//      0x13 : 320x200x8bits
	//      0x6a : 800x600x4bits
//	movw  $0x0003, %ax
//	int   $0x10

//	calll setup16

	movw  $SETUP_DATA_SEG, %ax
	movw  %ax, %es
	movl  $8, %es:SETUP_DISP_DEPTH    // ビット深度(グラフィックモード)
//	movl  $320, %es:SETUP_DISP_WIDTH  // 画面の幅
	movl  $80, %es:SETUP_DISP_WIDTH
//	movl  $200, %es:SETUP_DISP_HEIGHT // 画面の高さ
	movl  $25, %es:SETUP_DISP_HEIGHT
//	movl  $0x000a0000, %es:SETUP_DISP_VRAM // VRAM
	movl  $0x000b8000, %es:SETUP_DISP_VRAM

	xorl  %eax, %eax
	movb  $0x02, %ah
	int   $0x16
	movl  %eax, %es:SETUP_KEYB_LEDS // キーボードの LED

	// テキストモードのカーソル位置を取得する。
	movb  $0x03, %ah
	xorb  %bh, %bh
	int   $0x10
	xorl  %eax, %eax
	movb  %dl, %al
	movl  %eax, %es:SETUP_DISP_CURCOL
	movb  %dh, %al
	movl  %eax, %es:SETUP_DISP_CURROW

	// kernel file size
	popl  %eax
	movl  %eax, %es:SETUP_KERNFILE_SIZE

	movw  $SETUP_ACPI_MEMMAP, %di
	call  acpi_get_memmap
	//
	// ここは %eax != 0xffffffff でエラーチェックが必要
	//
	movl  %eax, %es:SETUP_ACPI_MEMMAP_COUNT

	lgdtl setup_gdtr
	lidtl setup_idtr
	movl  %cr0, %eax
	orl   $0x00000001, %eax
	movl  %eax, %cr0

	ljmpl $2*8, $start32

// search MP Floating Pointer Structure
// @param[in] %ds:(%si)  start adr of search range
// @param[in] %ds:(%di)  end adr of search range
scan_mpfps:
1:
	movl  %ds:(%si), %eax
	cmpl  $0x5f504d5f, %eax  // compare with "_MP_"
	je    2f
	addw  $4, %si
	cmpw  %di, %si
	jle   1b

	// found
	orw   %ax, %ax
	ret
2:

	.align 16
setup_gdt:
	// 3.4.3(p.3-12)
	.byte 0, 0, 0, 0, 0, 0, 0, 0
	// base  0x00000000
	// limit 0xffffff
	// [1]64bitコードセグメント
	.word 0xffff, 0x0000, 0x9a00, 0x00af
	// [2]32bitコードセグメント
	//    base : 0x00000
	.word 0xffff, 0x0000, 0x9a00, 0x00cf
	// [3]32bitデータセグメント
	//    base : 0x00000
	.word 0xffff, 0x0000, 0x9200, 0x00cf

	.align 4
setup_gdtr:
	.short 8*4 - 1
	.long setup_gdt
setup_idtr:
	.short 0
	.long 0

setup_msg:
	.ascii "setup executing...\r\n"
	.byte 0
a20isopen:
	.ascii "A20 gate opened.\r\n"
	.byte 0
a20isclose:
	.ascii "A20 gate closed.\r\n"
	.byte 0

/// 文字列を表示する
// %si : 表示するヌル終端文字列のポインタ
// %si, %ax, %bx は破壊される
display16:
	// lodsb : %al = *(%ds:%si++) 
        lodsb
	testb %al, %al
	jz    1f
	movb  $0x0e, %ah
	movb  $15, %bh  // 白
	int   $0x10
	jmp   display16
1:	ret

// ACPI経由で %es:%di にメモリマップを書き込む
// %eax でメモリマップの件数を返す。
// エラーの場合は %eax = 0xffffffff となる。
acpi_get_memmap:
	pushl %ebx
	pushl %ecx
	pushl %edx
	pushw %si

	xorw  %si, %si
1:
	movl  $24, %ecx         // sizeof buffer
	movl  $0x0000e820, %eax // function
	movl  $0x534d4150, %edx // 'SMAP'
	int   $0x15

	jc    2f                // error
	cmpl  $0x534d4150, %eax
	jne   2f                // if (%eax != 'SMAP') { goto error; }
	testl %ebx, %ebx
	jz    3f                // success end
	addw  $24, %di
	incw  %si
	jmp   1b

2:	// error end
	movl  $0xffffffff, %eax
	jmp   4f
3:	// success end
	xorl  %eax, %eax
	movw  %si, %ax
4:
	popw  %si
	popl  %edx
	popl  %ecx
	popl  %ebx
	ret


start32:
.code32
	movw  $3*8, %ax
	movw  %ax, %ds
	movw  %ax, %es
	movw  %ax, %fs
	movw  %ax, %gs
	movw  %ax, %ss
	movl  $SETUP_STACK_ADR, %esp

	// Enable PAE
	movl  $0x00000020, %eax
	movl  %eax, %cr4

	// ここで EFLAGS.IOPL を更新しておきたい。

	// Create Page Map

	// Zero clear PML4
	movl  $SETUP_PML4_PADR, %ebx
	movl  %ebx, %edi
	xorl  %eax, %eax
	movl  $(PAGE_SIZE * 1 / 4), %ecx
	rep stosl

	// アドレス空間の後半半分をカーネル用に割り当てるため、
	// PML4 の後半を KERN_PDPTE_PADR にマップする。
	leal  8*256(%ebx), %edi
	movl  $KERN_PDPTE_PADR + 0x007, %eax
	movl  $256, %ecx
1:	movl  %eax, (%edi)
	movl  $0, 4(%edi)
	addl  $8, %edi
	addl  $PAGE_SIZE, %eax
	loop  1b

	// カーネルメモリ空間の中に物理メモリマップを作る。
	// 0xffff800000000000 -> 0x0000000000000000
	// PDPTE
	// カーネル空間全体の PDPTE(1MiB) をここで割り当てておく。

	// PDPTE for physical memory map
	movl  $KERN_PDPTE_PADR, %edi
	movl  $SETUP_PDE_HEAD_PADR + 0x007, %eax
1:	movl  %eax, (%edi)
	movl  $0, 4(%edi)
	addl  $PAGE_SIZE, %eax
	addl  $8, %edi
	cmpl  $SETUP_PDE_TAIL_PADR - (PAGE_SIZE * 2) + 0x007, %eax
	jb    1b
	// Zero clear left PDPTE
2:	xorl  %eax, %eax
	movl  %eax, (%edi)
	movl  %eax, 4(%edi)
	addl  $8, %edi
	cmpl  $KERN_PDPTE_PADR + PAGE_SIZE * 256, %edi
	jb    2b

	//// 物理メモリマップ用の PDPTE
	//movl  $SETUP_PDE_HEAD_PADR + 0x007, (%edi)
	//movl  $0, 4(%edi)
	//movl  $SETUP_PDE_HEAD_PADR + PAGE_SIZE + 0x007, 8(%edi)
	//movl  $0, 12(%edi)
	//movl  $SETUP_PDE_HEAD_PADR + PAGE_SIZE*2 + 0x007, 16(%edi)
	//movl  $0, 20(%edi)
	//movl  $SETUP_PDE_HEAD_PADR + PAGE_SIZE*3 + 0x007, 24(%edi)
	//movl  $0, 28(%edi)
	//addl  $32, %edi
	//movl  $(PAGE_SIZE * 256 / 4) - 4, %ecx  // Zero clear left PDPTE
	//xorl  %eax, %eax
	//rep   stosl

	// PDE
	movl  $SETUP_PDE_HEAD_PADR, %edi
	movl  $0 + 0x183, %eax
1:	movl  %eax, (%edi)
	movl  $0, 4(%edi)
	addl  $0x00200000, %eax
	addl  $8, %edi
	// SETUP_PDE_TAIL_PADR までの２ページは次に使う。
	cmpl  $SETUP_PDE_TAIL_PADR - (PAGE_SIZE * 2), %edi
	jb    1b

	// 実行中のアドレスのメモリマップテーブルを作る。
	// PML4
	leal  0x007(%edi), %eax
	movl  %eax, (%ebx)
	// PDPTE
	leal  PAGE_SIZE + 0x007(%edi), %eax
	movl  %eax, (%edi)
	movl  $0, 4(%edi)
	// PDE
	addl  $PAGE_SIZE, %edi
	movl  $0 + 0x183, %eax
	movl  $32, %ecx  // 64MiB
1:	movl  %eax, (%edi)
	movl  $0, 4(%edi)
	addl  $8, %edi
	addl  $0x200000, %eax
	loop  1b

	// Set page table
	movl  %ebx, %cr3

	// Enable long mode
	movl  $0xc0000080, %ecx  // EFER MSR number
	rdmsr
	orw   $0x0100, %ax       // Long Mode Enable bit
	wrmsr

	// Enable paging and protect mode.
	movl  $0x80000001, %eax
	movl  %eax, %cr0

	// jump code64
	pushl $8        // code64 seg
	pushl $start64
	lret

start64:
.code64
	movw  $8, %ax
	movw  %ax, %ds
	movw  %ax, %es
	movw  %ax, %fs
	movw  %ax, %gs

	call prekernel 
	test %eax, %eax
	jnz  freeze

	movq $0xfffffffffffffff0, %rsp

	movq  $KERN_FINAL_VADR, %rax
	jmp *%rax

freeze:
	hlt
	jmp freeze
