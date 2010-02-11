/**
 * @file    arch/x86_64/boot/bootldr/loadkern.cpp
 * @version 0.0.0.2
 * @author  Kato.T
 * @brief   カーネルをメモリに読み込む。
 */
// (C) Kato.T 2010

#include "btypes.hpp"
#include "loadfat.hpp"

#include "boot.h"

asm (".code16gcc");

extern "C" unsigned long load_kernel(unsigned long);
extern "C" int bios_block_copy(
	unsigned long src_addr,
	unsigned long dest_addr,
	unsigned long words);
extern "C" void work_to_setup();

namespace {

/**
 * BIOS で 1 文字を表示する。
 * @param ch 表示する文字。
 * @param col 表示色。
 */
inline void bios_putch(char ch, _u8 col=15)
{
	const _u16 b = col;
	asm volatile ("int $0x10" : : "a" (0x0e00 | ch), "b" (b));
}

/**
 * BIOS で文字列を表示する。
 * @param str 表示するヌル終端文字列。
 * @param col 表示色。
 */
void bios_putstr(const char* str, _u8 col=15)
{
	while (*str) {
		bios_putch(*str++, col);
	}
}

const char chbase[] = "0123456789abcdef";

/**
 * ８ビット整数を１６進で表示する。
 * @param x 表示する整数。
 * @param col 表示色。
 */
void bios_put8_b16(_u8 x, _u8 col=15)
{
	bios_putch(chbase[x >> 4], col);
	bios_putch(chbase[x & 0xf], col);
}

/**
 * １６ビット整数を１６進で表示する。
 * @param x 表示する整数。
 * @param col 表示色。
 */
void bios_put16_b16(_u16 x, _u8 col=15)
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

} /* end namespace */

/**
 * カーネルをメモリに読み込む
 * @return カーネルのサイズを返す。
 */
unsigned long load_kernel(unsigned long x)
{
	bios_put16_b16((_u16)x);

	static const char couldnot_load[] =
		"Could not load kernel ROOTCORE.BIN from disk.\r\n";
	static const char copy_failed[] =
		"Block copy bios failed.\r\n";
	static const char no_kernel[] =
		"No kernel ROOTCORE.BIN found.\r\n";

	// ブートセクタからブートローダへ渡されるパラメータ
	const _u16 boot_drive
		= *reinterpret_cast<_u16*>(BOOT_DRIVE);
	const _u16 loaded_secs
		= *reinterpret_cast<_u16*>(BOOTSECT_LOADED_SECS);

	unsigned long kern_size = 0;

	// FAT12 ヘッダが BOOTSECT_ADR から始まる。
	fat_info fatinfo(boot_drive, reinterpret_cast<_u8*>(BOOTSECT_ADR));

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
		0, BOOTSECT_ADR + 512 * loaded_secs);

	// RDEから ROOTCORE.BIN を探す。
	dir_entry* ker_ent = fatinfo.find_from_dir("ROOTCORE" "BIN");

	if (ker_ent) {
		// ファイル本体の先頭クラスタ番号
		int clu = le16_to_cpu(ker_ent->start[0], ker_ent->start[1]);

		// カーネルの先頭64KiBを SETUP_SEG:0 へ読み込む。
		int is_error = fatinfo.read_to_segment(&clu, BOOTLDR_WORK_SEG);
		work_to_setup();
		if (is_error) {
			bios_putstr(couldnot_load);
			keyboard_and_reboot();
		}

		// カーネルの先頭64KiBを KERNEL_FINAL_ADR へも転送する。
		unsigned long dest = KERNEL_FINAL_ADR;
		const unsigned long block_size = 0x10000;
		int r = bios_block_copy(
			static_cast<unsigned long>(BOOTLDR_WORK_SEG) << 4,
			dest, block_size / 2);
		if (r != 0) {
			bios_putstr(copy_failed);
			bios_put8_b16(static_cast<_u8>(r & 0xff));
			keyboard_and_reboot();
		}
		dest += block_size;

		kern_size = le32_to_cpu(
			ker_ent->size[0], ker_ent->size[1],
			ker_ent->size[2], ker_ent->size[3]);
		unsigned long loaded = block_size;

		while (clu < 0x0ff8 && loaded < kern_size) {
			is_error = fatinfo.read_to_segment(&clu, BOOTLDR_WORK_SEG);
			if (is_error != 0) {
				bios_putstr(couldnot_load);
				keyboard_and_reboot();
			}
			is_error = bios_block_copy(
				static_cast<unsigned long>(BOOTLDR_WORK_SEG) << 4,
				dest, block_size / 2);
			if (is_error != 0) {
				bios_putstr(couldnot_load);
				keyboard_and_reboot();
			}
			else {
				dest += block_size;
				loaded += block_size;
			}
		}
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

	bios_putstr("bootldr end.\r\n");

	return kern_size;
}

