/// @file  boot.h
/// @brief 各ブートフェーズで使用するメモリアドレスの定義。
//
// (C) 2010 KATO Takeshi
//

#ifndef ARCH_X86_64_BOOT_INCLUDE_BOOT_H_
#define ARCH_X86_64_BOOT_INCLUDE_BOOT_H_

/**
 * メモリの使い方
 * - 0x0000:0400-0x0000:04ff  BIOS Data Area (BDA)
 * - 0x0000:7c00-0x0000:7dff  最初に bootsect がロードされる。
 * - 0x1000:0000-0x1000:01ff  bootsect と bootldr がスタックとして使う。
 * - 0x1000:0200-0x1000:03ff  bootsect が自分自身をコピーする。
 *                            ここはファイルシステムヘッダでもある。
 * - 0x1000:0400-0x1000:25ff  bootsect が bootsect の続きをロードする。
 *                            この中に bootldr がいる。
 */

/// BIOS が bootsect をロードするアドレス。
#define INIT_BOOTSECT_ADR   0x7c00

#define BOOTLDR_SEG        0x1000
/// bootsect が自分自身をコピーするアドレス。
#define BOOTSECT_ADR        0x0200
/// bootsect が bootldr をコピーするアドレス。
#define BOOTLDR_ADR         (BOOTSECT_ADR + 512)

/// bootsect と bootldr が使うスタック。
#define BOOT_STACK_SEG      BOOTLDR_SEG
#define BOOT_STACK_ADR      0x0200

/// ブートセクタがブートローダへ伝える情報のアドレス
// @{
#define BOOTSECT_LOADED_SECS (BOOT_STACK_ADR - 4)
#define BOOT_DRIVE           (BOOT_STACK_ADR - 2)
// @}

/// Boot loader working area.
#define BOOTLDR_WORK_SEG   0x2000

// カーネルの先頭64KiBを読み込むアドレス
#define SETUP_SEG          0x0000
#define SETUP_ADR           0x1000
#define SETUP_MAXSIZE       0x6000

// Stack address : 0x20000-0x30000
#define SETUP_STACK_ADR    0x30000

#define SETUP_DATA_SEG     0x8000
#define SETUP_DISP_DEPTH    0x0000
#define SETUP_DISP_WIDTH    0x0004
#define SETUP_DISP_HEIGHT   0x0008
#define SETUP_DISP_VRAM     0x000c
#define SETUP_KEYB_LEDS     0x0010
#define SETUP_MEMMAP_COUNT  0x0014
#define SETUP_DISP_CURROW   0x0018
#define SETUP_DISP_CURCOL   0x001c
#define SETUP_MEMMAP        0x0100

// カーネルの読み込み先アドレス
#define PHASE4_ADDR       0x100000

#define PH4_MEMMAP_BUF       0x50000
#define PH4_VIDEOTERM        0x60000

// 最終的なカーネルの実行アドレス
#define KERNEL_FINAL_ADR   0x100000


#endif  // Include guard

