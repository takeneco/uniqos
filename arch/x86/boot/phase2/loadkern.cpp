/* FILE : arch/x86/boot/phase2/loadkern.cpp
 * VER  : 0.0.4
 * LAST : 2009-05-24
 * (C) Kato.T 2008-2009
 *
 * カーネルをメモリに読み込む。
 */

#include "btypes.hpp"
#include "loadfat.hpp"

#include "boot.h"

asm (".code16gcc");

extern "C" _u32 load_kernel(_u32);
extern "C" int bios_block_copy(_u32, _u32, _u32);
extern "C" _u32 acpi_get_memmap(_u32, _u16, _u16);

/**
 * BIOS で 1 文字を表示する。
 * @param ch 表示する文字。
 * @param col 表示色。
 */
static inline void bios_putch(char ch, _u8 col=15)
{
	const _u16 b = col;
	asm volatile ("int $0x10" : : "a" (0x0e00 | ch), "b" (b));
}

/**
 * BIOS で文字列を表示する。
 * @param str 表示するヌル終端文字列。
 * @param col 表示色。
 */
static void bios_putstr(const char* str, _u8 col=15)
{
	while (*str) {
		bios_putch(*str++, col);
	}
}

static const char chbase[] = "0123456789abcdef";

/**
 * ８ビット整数を１６進で表示する。
 * @param x 表示する整数。
 * @param col 表示色。
 */
static void bios_put8_b16(_u8 x, _u8 col=15)
{
	bios_putch(chbase[x >> 4], col);
	bios_putch(chbase[x & 0xf], col);
}

/**
 * １６ビット整数を１６進で表示する。
 * @param x 表示する整数。
 * @param col 表示色。
 */
static void bios_put16_b16(_u16 x, _u8 col=15)
{
	bios_putch(chbase[(x >>12) & 0xf], col);
	bios_putch(chbase[(x >> 8) & 0xf], col);
	bios_putch(chbase[(x >> 4) & 0xf], col);
	bios_putch(chbase[x & 0xf], col);
}

/**
 * キーボード入力を待ってリブートする。
 */
void keyboard_and_reboot()
{
	asm volatile("int $0x16; int $0x19" : : "a"(0));
}

/**
 * カーネルをメモリに読み込む
 * @return カーネルのサイズを返す。
 */
_u32 load_kernel(_u32 x)
{
	bios_put16_b16((_u16)x);

	static const char couldnot_load[] =
		"Could not load kernel ROOTCORE.BIN from disk.\r\n";
	static const char copy_failed[] =
		"Block copy bios failed.\r\n";
	static const char no_kernel[] =
		"No kernel ROOTCORE.BIN found.\r\n";

	/*
	 * phase1 から phase2 へ渡されるパラメータ
	 * PHASE1_ADDR の前のアドレスに各パラメータが記録されている
	 */
	const _u16 boot_drive
		= *reinterpret_cast<_u16*>(PH1_2_BOOT_DRIVE);
	const _u16 loaded_secs
		= *reinterpret_cast<_u16*>(PH1_2_LOADED_SECS);

	_u32 kern_size;

	// FAT12 ヘッダが PHASE1_ADDR から始まる。
	fat_info fatinfo(boot_drive, reinterpret_cast<_u8*>(PHASE1_ADDR));

	/*
	 * ブートセグメント～FAT領域～RDE領域を読み込む。
	 * すでに読み込まれている範囲から足りない分だけをロードする。
	 */

	// RDE領域の終了位置
	_u16 re_endsec = fatinfo.rootent_endsec();

	// すでに読み込まれているアドレスの続きに読み込む。
	fatinfo.read_secs(
		loaded_secs,
		re_endsec - loaded_secs,
		0, PHASE1_ADDR + 512 * loaded_secs);

	// RDEから ROOTCORE.BIN を探す。
	dir_entry* ker_ent = fatinfo.find_from_dir("ROOTCORE" "BIN");

	if (ker_ent) {
		// ファイル本体の先頭クラスタ番号
		int start_clu
			= le16_to_cpu(ker_ent->start[0], ker_ent->start[1]);

		// カーネルの先頭64KiBを PHASE3_SEG:0 へ読み込む。
		int is_error = fatinfo.read_to_segment(&start_clu, PHASE3_SEG);
		if (is_error) {
			bios_putstr(couldnot_load);
			keyboard_and_reboot();
		}

		// カーネルの先頭64KiBを PHASE4_ADDR へも転送する。
		_u32 dest = PHASE4_ADDR;
		int r = bios_block_copy(PHASE3_SEG << 4, dest, 0x8000);
		if (r != 0) {
			bios_putstr(copy_failed);
			bios_put8_b16(static_cast<_u8>(r & 0xff));
			keyboard_and_reboot();
		}
		dest += 0x8000;
/*
		while (start_clu < 0x0ff8) {
			is_error = fatinfo.read_to_segment(
				&start_clu, 0x4000);
			if (is_error) {
				bios_putstr(couldnot_load);
				keyboard_and_reboot();
			}
			is_error = bios_block_copy(0x40000, dest, 0x8000);
			if (is_error) {
				bios_putstr(couldnot_load);
				keyboard_and_reboot();
			}
			else {
				dest += 0x8000;
			}
		}
*/

		kern_size = le32_to_cpu(
			ker_ent->size[0], ker_ent->size[1],
			ker_ent->size[2], ker_ent->size[3]);
	}
	else {
		bios_putstr(no_kernel);
		keyboard_and_reboot();
	}
/*
	// ACPI経由でメモリマップを取得する
	// メモリマップは 00020100 に格納する
	_u32 handle = 0;
	_u16 dest = 0x0100;
	do {
		handle = acpi_get_memmap(handle, 0x2000, dest);
		dest += 24;
	//	*(_u32*)0x00020120 = handle;
	//	break;
	} while (handle != 0);
*/

	return kern_size;
}

