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

// (C) 2009-2011 KATO Takeshi

#include "setup.h"

PAGE_SIZE = 4096


.section .entry

.globl _start
.globl a20isopen, a20isclose

_start:
.code16
	cli
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
/*
	// 8259
	// Protect I/O interrupt
	movb  $0xff, %al
	outb  %al, $0x21
	nop
	outb  %al, $0xa1
*/
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
	//movw  $0x0003, %ax
	//int   $0x10

	movw  $SETUP_DATA_SEG, %ax
	movw  %ax, %es
	movw  %ax, %fs

//
// %fs のセグメントへC++からアクセスするデータを集める。
//

	movl  $8, %fs:SETUP_DISP_DEPTH    // ビット深度(グラフィックモード)
	//movl  $320, %fs:SETUP_DISP_WIDTH  // 画面の幅
	movl  $80, %fs:SETUP_DISP_WIDTH
	//movl  $200, %fs:SETUP_DISP_HEIGHT // 画面の高さ
	movl  $25, %fs:SETUP_DISP_HEIGHT
	//movl  $0x000a0000, %fs:SETUP_DISP_VRAM // VRAM
	movl  $0x000b8000, %fs:SETUP_DISP_VRAM

	xorl  %eax, %eax
	movb  $0x02, %ah
	int   $0x16
	movl  %eax, %fs:SETUP_KEYB_LEDS // キーボードの LED

	// テキストモードのカーソル位置を取得する。
	movb  $0x03, %ah
	xorb  %bh, %bh
	int   $0x10
	xorl  %eax, %eax
	movb  %dl, %al
	movl  %eax, %fs:SETUP_DISP_CURCOL
	movb  %dh, %al
	movl  %eax, %fs:SETUP_DISP_CURROW

	// kernel file size
	popl  %eax
	movl  %eax, %fs:SETUP_KERNFILE_SIZE

	movw  $SETUP_ACPI_MEMMAP, %di
	call  acpi_get_memmap
	//
	// ここは %eax != 0xffffffff でエラーチェックが必要
	//
	movl  %eax, %fs:SETUP_ACPI_MEMMAP_COUNT

//
// collect MP info
//
	pushw %ds

	// search MP Floating Pointer Structure

	// search from EBDA(Extended BIOS Data Area) first 1KiB
	xorw  %ax,%ax
	movw  %ax, %ds
	movw  %ds:0x40e, %ax
	movw  %ax, %es
	xorw  %di,%di
	movw  $0x400, %cx
	call  scan_MPFPS
	jnc   1f

	// search from 0xf0000-0xfffff
	movw  $0xf000, %ax
	movw  %ax, %es
	xorw  %di,%di
	xorw  %cx,%cx  // means 0x10000
	call  scan_MPFPS
	jnc   1f

	// search from 0x9fc00-0xa0000
	movw  $0x9fc0, %ax
	movw  %ax, %es
	xorw  %di,%di
	movw  $0x400, %cx
	call  scan_MPFPS
	jnc   1f

	// MPFPS not found
	xorl  %eax, %eax
	movl  %eax, %fs:SETUP_MP_FLOATING_POINTER
	jmp   2f
1:
	// MPFPS found
	// search MP Configuration Table

	movl  %fs:SETUP_MP_FLOATING_POINTER+4, %eax   // ptr to MPCTH
	movw  %ax, %si
	xorw  %ax, %ax
	shrl  $4, %eax
	movw  %ax, %ds

	call  test_MPCT

2:
	popw  %ds

	lgdtl setup_gdtr
	lidtl setup_idtr
	movl  %cr0, %eax
	orl   $0x00000001, %eax
	movl  %eax, %cr0

	ljmpl $2*8, $start32


// search and copy MP Floating Pointer Structure.
//
// @param[in] %es:(%di)  start adr of search range
// @param[in] %cx        byte size of search range
// @return  not found if carry flag. else copy mpfps.
// @note  breaks %ax %cx %si %di %ds %es
scan_MPFPS:
SIZEOF_MPFPS=16
	// not searching last 12bytes, because mpfps is 16bytes.
	subw  $SIZEOF_MPFPS-4, %cx
	shrw  $2, %cx   // %cx /= 4
1:
	movl  $0x5f504d5f, %eax  // "_MP_"
	repne scasl   // compare %eax and %es:%di
	je    2f

	// not found
	stc
	ret

2:	// signature found
	pushw %cx
	movw  $SIZEOF_MPFPS, %cx  // sizeof mpfps
	movw  %es, %ax
	movw  %ax, %ds
	movw  %di, %si
	subw  $4, %si
	call  checksum8
	popw  %cx
	jnz   1b
	// checksum matched
	movw  $SIZEOF_MPFPS/2, %cx
	// checksum8 set %ds <- %es
	movw  %di, %si
	subw  $4, %si
	pushw %fs
	popw  %es
	movw  $SETUP_MP_FLOATING_POINTER, %di
	rep movsw 
	clc
	ret

// test and copy MP Configuration Table.
//
// @param[in] %ds:(%si)  start adr of MPCTH
// @note  destroy %eax %bl %cx %dx %si %di %es
test_MPCT:
TABLE_LENGTH_OFFSET=4
	movl  %ds:(%si), %eax
	cmpl  $0x504d4350, %eax
	jne   1f

	movw  %si, %dx
	movw  %ds:TABLE_LENGTH_OFFSET(%si), %cx
	call  checksum8
	jnz   1f

	movw  %dx, %si
	movw  %ds:TABLE_LENGTH_OFFSET(%si), %cx
	pushw %fs
	popw  %es
	movw  $SETUP_MP_CONFIGURATION_TABLE, %di
	rep movsw

	clc
	ret

1:	// MPCTH is ignore
	stc
	ret

// calc checksum.
//
// @param[in] %ds:(%si)  start adr of checksum range.
// @param[in] %cx        byte size of checksum range.
// @return  %bl is checksum value. zero flag is enable.
// @note  destroy %al %bl %cx
checksum8:
	xorb  %bl, %bl
1:
	lodsb
	addb  %al, %bl
	loop  1b

	ret


	.align 16
setup_gdt:
	// [0x00] nullセグメント
	.byte 0, 0, 0, 0, 0, 0, 0, 0

	// base  0x00000000
	// limit 0xffffff
	// [0x08] 64bitコードセグメント
	.word 0xffff, 0x0000, 0x9a00, 0x00af

	// [0x10] 32bitコードセグメント
	//    base : 0x00000
	.word 0xffff, 0x0000, 0x9a00, 0x00cf

	// [0x18] 32bitデータセグメント
	//    base : 0x00000
	.word 0xffff, 0x0000, 0x9200, 0x00cf

	// [0x20] 後で 64bit TSS になる
	.long 0, 0, 0, 0

	.align 4
setup_gdtr:
	.short 0x30 - 1
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
/*
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
*/
	movl  $KERN_PDPTE_PADR, %edi

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
	movl  $512, %ecx  // 1GiB
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
