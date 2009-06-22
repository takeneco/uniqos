/* FILE : arch/x86/boot/include/boot.hpp
 * VER  : 0.0.2
 * LAST : 2009-05-18
 * (C) Kato.T 2009
 */

#ifndef _ARCH_X86_BOOT_INCLUDE_BOOT_H
#define _ARCH_X86_BOOT_INCLUDE_BOOT_H

// 各フェーズの読み込み先アドレス

// ブートセクタ
#define PHASE1_ADDR 0x7c00

#define PH1_2_LOADED_SECS (PHASE1_ADDR - 4)
#define PH1_2_BOOT_DRIVE  (PHASE1_ADDR - 2)

// ブートローダ
#define PHASE2_ADDR 0x7e00

// カーネルの先頭64KiBを読み込むセグメント(phase3)
#define PHASE3_SEG 0x3000

#define PH3_TO_PH4_PARAM_SEG 0x8000
#define PARAM_DISP_DEPTH      0x0000
#define PARAM_DISP_WIDTH      0x0004
#define PARAM_DISP_HEIGHT     0x0008
#define PARAM_DISP_VRAM       0x000c
#define PARAM_KEYB_LEDS       0x0010
#define PARAM_MEMMAP_COUNT    0x0014
#define PARAM_MEMMAP          0x0100

// カーネルの読み込み先アドレス
#define PHASE4_ADDR 0x100000

#endif // _ARCH_X86_BOOT_INCLUDE_BOOT_H
