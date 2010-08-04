// @file    arch/x86_64/kernel/setup/setup.h
// @author  Kato Takeshi
// @brief   アセンブラとC++で共有するパラメータの定義
//
// (C) 2010 Kato Takeshi

#ifndef _ARCH_X86_64_BOOT_INCLUDE_BOOT_H_
#define _ARCH_X86_64_BOOT_INCLUDE_BOOT_H_


/**
 * @brief  Constantly phisical address map.
 *
 * - 00001000 - 00006fff  Kernel head code.
 * - 00010000 - 0001ffff  Setup stack.
 * - 00020000 - 0002ffff  Setup memmgr buffer.
 * - 00030000 - 00032fff  Setup address tr table PML4.
 * - 00080000 - 0008ffff  Setup collect params store by BIOS.
 * - 00100000 - 00ffffff  Compressed kernel body (15MiB).
 * - 01000000 - 010fffff  Kernel constant use PDPTEs (1MiB).
 * - 01100000 - 011fffff  Kernel constant use PDEs (1MiB).
 * - 01200000 -           Extracted kernel body.
 */

// Stack address : 0x10000-0x1ffff
#define SETUP_STACK_ADR             0x20000

// Memory management buffer
#define SETUP_MEMMGR_ADR            0x20000
#define SETUP_MEMMGR_SIZE           0x10000

/// Address tr table for setup.
#define SETUP_PML4_PADR             0x30000
/// セットアップ中に作るメモリマップテーブルの格納アドレス
#define SETUP_PDE_HEAD_PADR         (SETUP_PML4_PADR + 0x1000)
#define SETUP_PDE_TAIL_PADR         0x7ffff

// セットアップ中にBIOSから集めたデータを格納するアドレス
#define SETUP_DATA_SEG              0x8000
#define SETUP_DISP_DEPTH             0x0000
#define SETUP_DISP_WIDTH             0x0004
#define SETUP_DISP_HEIGHT            0x0008
#define SETUP_DISP_VRAM              0x000c
#define SETUP_KEYB_LEDS              0x0010
#define SETUP_MEMMAP_COUNT           0x0014
#define SETUP_DISP_CURROW            0x0018
#define SETUP_DISP_CURCOL            0x001c
#define SETUP_KERNFILE_SIZE          0x0020
// ACPIから取得したメモリマップ
#define SETUP_MEMMAP                 0x0100
// Setupフェーズ終了後のメモリマップ
#define SETUP_MEMMAP_DUMP            0x0400

/// Full kernel load address by bootloader.
#define SETUP_KERN_ADR             0x100000

/// Kernel extract temporaly address.
#define KERN_EXTTMP_ADR          0x00200000
#define KERN_EXTTMP_SIZE         0x00100000

/// Kernel constantly use paging table.
/// @{
/// Page Directory Pointer Table Entry (1MiB).
#define KERN_PDPTE_PADR          0x01000000
/// Page Directry Entry (size variable).
#define KERN_PDE_PADR            0x01100000
/// @}

/// Reserved memory address from 0.
#define SETUP_MEMMGR_RESERVED_PADR 0x013fffff

/// Finally kernel execute virtual address.
#define KERN_FINAL_VADR  0xffffffff00000000
#define KERN_FINAL_SIZE  0x0000000100000000


#endif  // Include guard.

