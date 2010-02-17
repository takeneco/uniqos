/**
 * @file    arch/x86_64/kernel/setup/setup.h
 * @version 0.0.0.1
 * @author  Kato.T
 * @brief   
 */
// (C) Kato.T 2010

#ifndef _ARCH_X86_64_BOOT_INCLUDE_BOOT_H_
#define _ARCH_X86_64_BOOT_INCLUDE_BOOT_H_

/**
 * - 00001000 - 00006fff  Kernel head
 * - 00020000 - 0002ffff  Setup stack
 * - 00030000 - 0003ffff  Setup memmgr buffer
 * - 00080000 - 0008ffff  Setup params
 * - 00100000 -           kernel body
 */

// カーネルの先頭64KiBを読み込むアドレス
#define SETUP_SEG          0x1000
#define SETUP_ADR           0x0000

// Stack address : 0x20000-0x2ffff
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

#define MEMMGR_MEMMAP_ADR  0x30000
#define MEMMGR_MEMMAP_SIZE 0x10000

// カーネルの読み込み先アドレス
#define SETUP_KERN_ADR    0x100000

/// Address tr table for kernel.
#define KERN_PML4_ADR     0x800000

// 最終的なカーネルの実行アドレス
#define KERNEL_FINAL_ADR  0x100000

#endif  // Include guard.

