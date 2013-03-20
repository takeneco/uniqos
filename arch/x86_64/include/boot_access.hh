/// @file  boot_access.h
/// @brief セットアップコードとカーネルが共有する宣言
//
// (C) 2010-2011 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_BOOT_ACCESS_HH_
#define ARCH_X86_64_INCLUDE_BOOT_ACCESS_HH_


/**
 * @brief  Constantly phisical address map.
 *
 * - 00000000-000004ff  Reserved by BIOS, etc...
 * - 00000500-00007fff  Kernel head code including Setup.
 * - 00018000-0001bfff  Setup stack.
 * - 0001c000-0001ffff  Setup memory allocate buffer.
 * - 00020000-0002ffff  Setup log buffer.
 * - 00030000-00030fff  Kernel PML4 table.
 * - 00031000-0007ffff  Setup address tr table.
 * - 00080000-0008ffff  Setup collect params store by BIOS.
 * - 00100000-00ffffff  Compressed kernel body (15MiB).
 * - 01000000-010fffff  Kernel constant use PDPTEs (1MiB).
 * - 01100000-011fffff  Kernel constant use PDEs (1MiB).
 * - 01200000-          Extracted kernel body.
 */

// Stack address : 0x18000-0x1bfff
#define SETUP_STACK_ADR             0x1c000

// Memory management buffer
#define SETUP_MEMMGR_ADR            0x1c000
#define SETUP_MEMMGR_SIZE           0x04000

// log buffer.
#define SETUP_LOGBUF_SEG            0x2000
#define SETUP_LOGBUF_ADR             0x0000
#define SETUP_LOGBUF_SIZE           0x10000

/// Address tr table for setup.
#define SETUP_PML4_PADR             0x30000
/// セットアップ中に作るメモリマップテーブルの格納アドレス
#define SETUP_PDE_HEAD_PADR         (SETUP_PML4_PADR + 0x1000)
#define SETUP_PDE_TAIL_PADR         0x7ffff

// setup から kernel へ渡すデータを格納するアドレス
#define SETUP_DATA_SEG              0x8000
#define SETUP_DISP_DEPTH             0x0000
#define SETUP_DISP_WIDTH             0x0004
#define SETUP_DISP_HEIGHT            0x0008
#define SETUP_DISP_VRAM              0x000c
#define SETUP_KEYB_LEDS              0x0010
#define SETUP_ACPI_MEMMAP_COUNT      0x0014
#define SETUP_DISP_CURROW            0x0018
#define SETUP_DISP_CURCOL            0x001c
#define SETUP_KERNFILE_SIZE          0x0020
#define SETUP_FREEMEM_DUMP_COUNT     0x0024
#define SETUP_USEDMEM_DUMP_COUNT     0x0028
// end of log buffer. offset from SETUP_LOGBUF_START.
#define SETUP_LOGBUF_CUR             0x002c
/// ACPIから取得したメモリマップ
#define SETUP_ACPI_MEMMAP            0x0100
/// Setupフェーズ終了後の空きメモリ情報
#define SETUP_FREEMEM_DUMP           0x0400
/// Setupフェーズ終了後の使用メモリ情報
#define SETUP_USEDMEM_DUMP           0x0500


#endif  // include guard

