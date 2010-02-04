/**
 * @file    arch/x86_64/boot/include/boot.h
 * @version 0.0.0.1
 * @author  Kato.T
 * @brief   各ブートフェーズで使用するメモリアドレスの定義。
 */
// (C) Kato.T 2010

#ifndef _ARCH_X86_64_BOOT_INCLUDE_BOOT_H_
#define _ARCH_X86_64_BOOT_INCLUDE_BOOT_H_

/// Boot sector address.
#define BOOTSECT_ADR        0x7c00

/// ブートセクタがブートローダへ伝える情報のアドレス
// @{
#define BOOTSECT_LOADED_SECS (BOOTSECT_ADR - 4)
#define BOOT_DRIVE           (BOOTSECT_ADR - 2)
// @}

/// Boot loader address.
#define BOOTLDR_SEG        0x0000
#define BOOTLDR_ADR         0x7e00

/// Boot loader working area.
#define BOOTLDR_WORK_SEG   0x2000

// カーネルの先頭64KiBを読み込むアドレス
#define SETUP_SEG          0x1000
#define SETUP_ADR           0x0000

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

#endif  // _ARCH_X86_64_BOOT_INCLUDE_BOOT_H_

