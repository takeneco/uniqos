/**
 * @file    arch/x86_64/kernel/setup/setup.h
 * @author  Kato.T
 *
 * (C) Kato.T 2010
 */

#ifndef _ARCH_X86_64_BOOT_INCLUDE_BOOT_H_
#define _ARCH_X86_64_BOOT_INCLUDE_BOOT_H_


/**
 * @brief  Constantly phisical address map.
 *
 * - 00001000 - 00006fff  Kernel head code.
 * - 00010000 - 0001ffff  Setup stack.
 * - 00020000 - 0002ffff  Setup memmgr buffer.
 * - 00030000 - 00032fff  Setup address tr table.
 * - 00080000 - 0008ffff  Setup collect params by BIOS.
 * - 00100000 - 00ffffff  Compressed kernel body (16MiB).
 * - 01000000 - 010fffff  Kernel constant use PDPTEs (1MiB).
 * - 01100000 - 011fffff  Kernel constant use PDEs (1MiB).
 * - 01200000 -           Extracted kernel body.
 */

// Stack address : 0x10000-0x1ffff
#define SETUP_STACK_ADR             0x20000

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
#define SETUP_MEMMAP                 0x0100
#define SETUP_MEMMAP_DUMP            0x0400

#define MEMMGR_MEMMAP_ADR           0x20000
#define MEMMGR_MEMMAP_SIZE          0x10000

/// Full kernel load address by bootloader.
#define SETUP_KERN_ADR             0x100000

/// Address tr table for setup.
#define SETUP_PML4_ADR              0x30000

/// Kernel extract temporaly address.
#define KERN_EXTTMP_ADR          0x00200000
#define KERN_EXTTMP_SIZE         0x00100000

/// Kernel constantly use paging table.
#define KERN_PDPTE_PADR          0x01000000
#define KERN_PDE_PADR            0x01100000

/// Reserved memory address from 0.
#define MEMMGR_RESERVED_PADR     0x011fffff

/// Finally kernel execute virtual address.
#define KERN_FINAL_VADR  0xffffffff00000000
#define KERN_FINAL_SIZE  0x0000000100000000


#endif  // Include guard.

