/**
 * @file    arch/x86/boot/phase4/main.c
 * @version 0.0.5
 * @date    2009-07-05
 * @author  Kato.T
 *
 * 32ビットプロテクトモード移行後からカーネル本体へのジャンプまで
 */
// (C) Kato.T 2009

#include "native.hpp"
#include "phase4.hpp"


extern "C" void setup_main(_u8* ker_src, _u32 ker_dest);

extern "C" void _int21_handler(void);
extern "C" void _int23_handler(void);
extern "C" void _int24_handler(void);

static void putu8(_u8* dest, _u8 c)
{
	dest[0] = c < 10 ? '0' + c : 'a' + c - 10;
	dest[1] = 0x0f;
}

static void putu64(_u8* dest, _u64 n)
{
	putu8(dest + 30, (char)((n & 0x000000000000000fULL)));
	putu8(dest + 28, (char)((n & 0x00000000000000f0ULL) >> 4));
	putu8(dest + 26, (char)((n & 0x0000000000000f00ULL) >> 8));
	putu8(dest + 24, (char)((n & 0x000000000000f000ULL) >> 12));
	putu8(dest + 22, (char)((n & 0x00000000000f0000ULL) >> 16));
	putu8(dest + 20, (char)((n & 0x0000000000f00000ULL) >> 20));
	putu8(dest + 18, (char)((n & 0x000000000f000000ULL) >> 24));
	putu8(dest + 16, (char)((n & 0x00000000f0000000ULL) >> 28));
	putu8(dest + 14, (char)((n & 0x0000000f00000000ULL) >> 32));
	putu8(dest + 12, (char)((n & 0x000000f000000000ULL) >> 36));
	putu8(dest + 10, (char)((n & 0x00000f0000000000ULL) >> 40));
	putu8(dest +  8, (char)((n & 0x0000f00000000000ULL) >> 44));
	putu8(dest +  6, (char)((n & 0x000f000000000000ULL) >> 48));
	putu8(dest +  4, (char)((n & 0x00f0000000000000ULL) >> 52));
	putu8(dest +  2, (char)((n & 0x0f00000000000000ULL) >> 56));
	putu8(dest +  0, (char)((n & 0xf000000000000000ULL) >> 60));
}

extern "C" void _int21h()
{
}

extern "C" void _int23h()
{
}

extern "C" void _int24h()
{
	native_outb(0x64, 0x0020);

	get_video_term()
	->puts("com1:int=")->putu32x(native_inb(0x03fa))
	->puts(",sta=")->putu32x(native_inb(0x03fd))
	->puts(",dat=")->putu32x(native_inb(0x03f8))
	->puts(",ien=")->putu32x(native_inb(0x03f9))->putc('\n');
}

// グローバルディスクリプタテーブルのエントリ (_u64)
// 下巻：システムプログラミングガイド p.3-12
#define GDT_ENTRY(base, limit, flags)     ( \
	(((base)  & 0xff000000ULL) << 32) | \
	(((base)  & 0x00ffffffULL) << 16) | \
	(((limit) & 0x000f0000ULL) << 32) | \
	 ((limit) & 0x0000ffffULL)        | \
	(((flags) & 0x0000f0ffULL) << 40) )

// 最小限のGDTフラグ
// 　グラニュラリティ(G)：有効
// 　デフォルトのオペレーションサイズ(D/B)：３２ビット
// 　セグメント存在(P)
// 　ディスクリプタタイプ：コードまたはデータ（非システム）
#define GDT_MINFLAGS  0xc090

// ディスクリプタの特権レベル(DPL)
// ０が最高レベル
#define GDT_DPL0 0
#define GDT_DPL1 (1 << 5)
#define GDT_DPL2 (2 << 5)
#define GDT_DPL3 (3 << 5)

// ディスクリプタタイプ
#define GDT_R    0x0000
#define GDT_RW   0x0002
#define GDT_X    0x0008
#define GDT_RX   0x000a

static void load_gdt()
{
	static _u64 gdt[] = {
		GDT_ENTRY(0, 0, 0),
		GDT_ENTRY(0x00000, 0xfffff, GDT_MINFLAGS | GDT_DPL0 | GDT_RX),
		GDT_ENTRY(0x00000, 0xfffff, GDT_MINFLAGS | GDT_DPL0 | GDT_RW),
		GDT_ENTRY(0x00000, 0xfffff, GDT_MINFLAGS | GDT_DPL3 | GDT_RX),
		GDT_ENTRY(0x00000, 0xfffff, GDT_MINFLAGS | GDT_DPL3 | GDT_RW)
	};
	gidt_ptr64 gdtptr;

	init_gidt_ptr64(&gdtptr, sizeof gdt, gdt);
	native_lgdt(&gdtptr);
}

// ゲートディスクリプタテーブル(IDT)のエントリ (_u64)
// 下巻：システムプログラミングガイド p.5-16
#define IDT_ENTRY(offset, selec, flags)    ( \
	((reinterpret_cast<_u64>(offset) & 0xffff0000ULL) << 32) | \
	 (reinterpret_cast<_u64>(offset) & 0x0000ffffULL)        | \
	(((selec)  & 0x0000ffffULL) << 16) | \
	(((flags)  & 0x0000ffffULL) << 32) )

// 最小限の割り込みIDTフラグ
// 　セグメント存在(P)
// 　ゲートのサイズ(D)：３２ビット
#define IDT_MINFLAGS 0x8e00

// ディスクリプタの特権レベル(DPL)
// ０が最高レベル
#define IDT_DPL0 0
#define IDT_DPL1 (1 << 13)
#define IDT_DPL2 (2 << 13)
#define IDT_DPL3 (3 << 13)

void load_idt(memmgr* mm)
{
	const int idt_len = 256;

	// 確保したまま、解放しない。
	_u64* idt = new(mm) _u64[idt_len];
	gidt_ptr64 idtptr;

	for (int i = 0; i < idt_len; i++) {
		idt[i] = 0ULL;
	}
	idt[0x21] = IDT_ENTRY(_int21_handler, 1 * 8, IDT_MINFLAGS | IDT_DPL0);
	idt[0x2c] = IDT_ENTRY(_int21_handler, 1 * 8, IDT_MINFLAGS | IDT_DPL0);
	idt[0x23] = IDT_ENTRY(_int23_handler, 1 * 8, IDT_MINFLAGS | IDT_DPL0);
	idt[0x24] = IDT_ENTRY(_int24_handler, 1 * 8, IDT_MINFLAGS | IDT_DPL0);

	init_gidt_ptr64(&idtptr, sizeof *idt * idt_len, idt);
	native_lidt(&idtptr);
}

void wait_KBC_sendready()
{
	for (;;) {
		if ((native_inb(0x0064) & 0x02) == 0)
			break;
	}
}

#define CR0_CACHE_DISABLE 0x60000000

inline _u8 native_get_mem(_u32 p) {
	_u8 c;
	asm volatile("movb %1, %0" : "=r" (c)
		: "m" (*reinterpret_cast<const _u8*>(p)));
	return c;
}
inline void native_set_mem(_u32 p, _u8 c) {
	asm volatile("movb %1, %0"
		: "=m" (*reinterpret_cast<_u8*>(p)) : "r" (c));
}

void memtest()
{
	_u64 cr0;
	_u32 p, h;

	cr0 = native_get_cr0_64();
	native_set_cr0_64(cr0 | CR0_CACHE_DISABLE);

	p = 0;
	h = 0x80000000;
	for ( ; h != 0; ) {
		_u8 c;
		_u32 ph = p + h;
		c = native_get_mem(ph);
		native_set_mem(ph, c + 1);

		if (c + 1 == native_get_mem(ph)) {
			p += h;
		}
		h >>= 1;

		native_set_mem(p, c);
	}

	native_set_cr0_64(cr0);
}

void setup_main(_u8* ker_src, _u32 ker_size)
{
	memmgr mm;

	memmgr_init(&mm);

	_u8* param = reinterpret_cast<_u8*>(PH3_4_PARAM_SEG << 4);

	video_term* cons = new(&mm) video_term;
	set_video_term(cons);

	cons->init(
		*reinterpret_cast<_u32*>(&param[PH3_4_DISP_WIDTH]),
		*reinterpret_cast<_u32*>(&param[PH3_4_DISP_HEIGHT]),
		*reinterpret_cast<_u32*>(&param[PH3_4_DISP_VRAM]));
	cons->set_cur(
		*reinterpret_cast<_u32*>(&param[PH3_4_DISP_CURROW]),
		*reinterpret_cast<_u32*>(&param[PH3_4_DISP_CURCOL]));

	load_gdt();
	load_idt(&mm);

	// PICの初期化
	enum {
		PIC0_IMR = 0x0021,
		PIC0_ICW1 = 0x0020,
		PIC0_ICW2 = 0x0021,
		PIC0_ICW3 = 0x0021,
		PIC0_ICW4 = 0x0021,
		PIC1_IMR = 0x00a1,
		PIC1_ICW1 = 0x00a0,
		PIC1_ICW2 = 0x00a1,
		PIC1_ICW3 = 0x00a1,
		PIC1_ICW4 = 0x00a1,
	};
	// すべての割り込みを受け付けない。
	native_outb(0xff, PIC0_IMR);
	native_outb(0xff, PIC1_IMR);

	// PIC0

	// エッジトリガモード
	native_outb(0x11, PIC0_ICW1);
	// IRQ0-7 を INT20-27 で受け取る。
	native_outb(0x20, PIC0_ICW2);
	// PIC1 は PIC0 の IRQ2 にカスケード接続されている。
	native_outb(1 << 2, PIC0_ICW3);
	// ノンバッファモード
	native_outb(0x01, PIC0_ICW4);

	// PIC1

	// エッジトリガモード
	native_outb(0x11, PIC1_ICW1);
	// IRQ8-15 を INT28-2f で受け取る。
	native_outb(0x28, PIC1_ICW2);
	// PIC1 は PIC0 の IRQ2 にカスケード接続されている。
	native_outb(2, PIC1_ICW3);
	// ノンバッファモード
	native_outb(0x01, PIC1_ICW4);

	// 割り込み禁止
	// PIC0 : 0x02 - キーボード禁止
	// PIC0 : 0x04 - IRQ2カスケード禁止
	// PIC0 : 0x08 - COM2禁止
	// PIC0 : 0x10 - COM1禁止
	native_outb(0xe1, PIC0_IMR);
	// PIC1 : 0x10 - マウス禁止
	native_outb(0xef, PIC1_IMR);

	native_sti();

	wait_KBC_sendready();
	native_outb(0xd4, 0x0064);
	wait_KBC_sendready();
	native_outb(0xf4, 0x0060);

	// 割り込み設定
//	native_outb(0x00, 0x03f9);
	native_outb(0x0f, 0x03f9);

	memtest();

	term_chain tc;
	tc.init();

	com_term* ct = new(&mm) com_term;
	ct->init(com_term::COM1_BASE, com_term::COM1_IRQ);

	tc.add_term(ct);

	tc.puts("This is a test message.");

	int z = native_inb(0x03f8);

	_u8* ker_dest = reinterpret_cast<_u8*>(KERNEL_FINAL_ADDR);
	lzma_decode(&mm, ker_src, ker_size, ker_dest, 0x100000);

	cons->putu64x(*(_u64*)ker_dest);
/*
	for (;;) {
		native_hlt();
	}
*/
}
