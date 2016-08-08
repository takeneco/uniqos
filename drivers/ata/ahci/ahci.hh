/// @file   ahci.hh
/// @brief  AHCI driver declarations.

//  Uniqos  --  Unique Operating System
//  (C) 2015 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef AHCI_HH_
#define AHCI_HH_

#include <core/device.hh>
#include <core/driver.hh>
#include <core/io_node.hh>
#include <core/mempool.hh>
#include <core/pci.hh>
#include <core/spinlock.hh>
#include <util/chain.hh>
#include <util/cheap_alloc.hh>


namespace ahci {

enum {
	CMD_HDR_ALIGN  = 1024,
	CMD_HDR_NR     = 32,          ///< Max Command Headers in Command List
	RX_FIS_ALIGN   = 256,
	RX_FIS_SZ      = 256,
	CMD_TBL_ALIGN  = 128,
	PRDT_NR        = 56,          ///< PRDTs in Command Table

	TABLE_ALLOC_ENTS      = 110,
	TABLE_ALLOC_SLOT      = 0,
	TABLE_ALLOC_SLOT_MASK = 1 << TABLE_ALLOC_SLOT,

	CAP_S64A       = 0x80000000,
	CAP_SNCQ       = 0x40000000,
	CAP_SSNTF      = 0x20000000,
	CAP_SMPS       = 0x10000000,
	CAP_SSS        = 0x08000000,
	CAP_SALP       = 0x04000000,
	CAP_SAL        = 0x02000000,
	CAP_SCLO       = 0x01000000,
	CAP_ISS        = 0x00f00000,
	// reserved      0x00080000,
	CAP_SAM        = 0x00040000,
	CAP_SPM        = 0x00020000,
	CAP_FBSS       = 0x00010000,
	CAP_PMD        = 0x00008000,
	CAP_SSC        = 0x00004000,
	CAP_PSC        = 0x00002000,
	CAP_NCS_MASK   = 0x00001f00,  CAP_NCS_SHIFT  = 8,
	CAP_CCCS       = 0x00000080,
	CAP_EMS        = 0x00000040,
	CAP_SXS        = 0x00000020,
	CAP_NP         = 0x0000001f,

	PxCMD_ICC_MASK  = 0xf0000000,
	PxCMD_ASP       = 0x08000000,
	PxCMD_ALPE      = 0x04000000,
	PxCMD_DLAE      = 0x02000000,
	PxCMD_ATAPI     = 0x01000000,
	PxCMD_APSTE     = 0x00800000,
	PxCMD_FBSCP     = 0x00400000,
	PxCMD_ESP       = 0x00200000,
	PxCMD_CPD       = 0x00100000,
	PxCMD_MPSP      = 0x00080000,
	PxCMD_HPCP      = 0x00040000,
	PxCMD_PMA       = 0x00020000,
	PxCMD_CPS       = 0x00010000,
	PxCMD_CR        = 0x00008000,
	PxCMD_FR        = 0x00004000,
	PxCMD_MPSS      = 0x00002000,
	PxCMD_CCS_MASK  = 0x00001f00,
	PxCMD_FRE       = 0x00000010,
	PxCMD_CLO       = 0x00000008,
	PxCMD_POD       = 0x00000004,
	PxCMD_SUD       = 0x00000002,
	PxCMD_ST        = 0x00000001,
};

struct FIS_H2D
{
	// DWORD 0
	u8 fis_type;

	u8 pmport:4;  // Port multiplier
	u8 rsv0:3;    // Reserved
	u8 c:1;       // 1: Command, 0: Control

	u8 command;   // Command register
	u8 featurel;  // Feature register, 7:0

	// DWORD 1
	u8 lba0;      // LBA low register, 7:0
	u8 lba1;      // LBA mid register, 15:8
	u8 lba2;      // LBA high register, 23:16
	u8 device;    // Device register

	// DWORD 2
	u8 lba3;      // LBA register, 31:24
	u8 lba4;      // LBA register, 39:32
	u8 lba5;      // LBA register, 47:40
	u8 featureh;  // Feature register, 15:8

	// DWORD 3
	u8 countl;    // Count register, 7:0
	u8 counth;    // Count register, 15:8
	u8 icc;       // Isochronous command completion
	u8 control;   // Control register

	// DWORD 4
	u8 rsv1[4];   // Reserved
};

struct FIS_D2H
{
	// DWORD 0
	u8 fis_type;

	u8 pmport:4;  // Port multiplier
	u8 rsv0:2;    // Reserved
	u8 i:1;       // Interrupt bit
	u8 rsv1:1;    // Reserved;
	u8 status;    // Status register
	u8 error;     // Error register

	// DWORD 1
	u8 lba0;      // LBA low register, 7:0
	u8 lba1;      // LBA mid register, 15:8
	u8 lba2;      // LBA high register, 23:16
	u8 device;    // Device register

	// DWORD 2
	u8 lba3;      // LBA register, 31:24
	u8 lba4;      // LBA register, 39:32
	u8 lba5;      // LBA register, 47:40
	u8 rsv2;      // Reserved

	// DWORD 3
	u8 countl;    // Count register, 7:0
	u8 counth;    // Count register, 15:8
	u8 rsv3[2];   // Reserved

	// DWORD 4
	u8 rsv4[4];   // Reserved
};

using HBA_MEM_REGS = volatile struct _HBA_MEM_REGS
{
	u32 cap;        // 0x00  Host Capabilities
	u32 ghc;        // 0x04  Global Host Control
	u32 is;         // 0x08  Interrupt Status
	u32 pi;         // 0x0C  Ports implemented
	u32 vs;         // 0x10  Version
	u32 ccc_ctl;    // 0x14  Command Completion Coalescing Control
	u32 ccc_ports;  // 0x18  Command Completion Coalsecing Ports
	u32 em_loc;     // 0x1C  Enclosure Management Location
	u32 em_ctl;     // 0x20  Enclosure Management Control
	u32 cap2;       // 0x24  Host Capabilities Extended 
	u32 bohc;       // 0x28  BIOS/OS Handoff Control and Status

	u8  reserved[0x60 - 0x2C];  // 0x2C
	u8  reserved_for_nvmhci[0xA0 - 0x60];  // 0x60
	u8  vendor[0x100 - 0xA0];  // 0xA0  Vendor Specific registers

	using PORT = volatile struct _PORT
	{
		u32 clb;     // 0x00  Command List Base Address
		u32 clbu;    // 0x04  Command List Base Address Upper 32-Bits
		u32 fb;      // 0x08  FIS Base Address
		u32 fbu;     // 0x0C  FIS Base Address Upper 32-Bits
		u32 is;      // 0x10  Interrupt Status
		u32 ie;      // 0x14  Interrupt Enable
		u32 cmd;     // 0x18  Command and Status

		u32 reserved1;

		u32 tfd;     // 0x20  Task File Data
		u32 sig;     // 0x24  Signature
		u32 ssts;    // 0x28  Serial ATA Status (SCR0: SStatus)
		u32 sctl;    // 0x2C  Serial ATA Control(SCR2: SControl)
		u32 serr;    // 0x30  Serial ATA Error (SCR1: SError)
		u32 sact;    // 0x34  Serial ATA Active (SCR3: SActive)
		u32 ci;      // 0x38  Command Issue
		u32 sntf;    // 0x3C  Serial ATA Notification (SCR4: SNotification)
		u32 fbs;     // 0x40  FIS-based Switching Control
		u32 devslp;  // 0x44  Device Sleep

		u8 reserved2[0x70 - 0x48];  // 0x48

		u8 vs[0x80 - 0x70];  // 0x70  Vendor Specific
	};

	PORT port[32];
};

struct COMMAND_HEADER
{
	u8  cfl:5;
	u8  a:1;
	u8  w:1;
	u8  p:1;

	u8  r:1;
	u8  b:1;
	u8  c:1;
	u8  reserved1:1;
	u8  pmp:4;

	u16 prdtl;

	volatile u32 prdbc;

	u32 ctba;
	u32 ctbau0;

	u32 reserved2[4];
};

struct COMMAND_TABLE
{
	u8  cfis[64];  // 0x00  Command FIS
	u8  acmd[16];  // 0x40  ATAPI Command (12 or 16 bytes)
	u8  reserved1[0x80 - 0x50];  // 0x50

	struct PRDT
	{
		u32  dba;
		u32  dbau;
		u32  reserved1;
		u32  dbc:22;
		u32  reserved2:9;
		u32  i:1;
	};

	PRDT prdt[];   // 0x80  Physical Region Descriptor Table
	               // (up to 65535 entries)
};

using table_alloc = cheap_alloc<TABLE_ALLOC_ENTS>;

class ahci_device_io_node;
class ahci_device;
class ahci_hba;
class ahci_driver;

class ahci_device_io_node : public io_node
{
public:
	ahci_device_io_node(ahci_device* owner);

	cause::pair<uptr> on_Write(
	    offset off, const void* data, uptr bytes);

private:
	ahci_device* dev;
};

class ahci_device : public device
{
public:
	ahci_device(ahci_driver* ahcidriver, ahci_hba* ahcihba, int hbaport);
	~ahci_device();

public:
	cause::t setup();
	cause::t start();
	cause::t stop();

	u16 get_segment_bits() const { return segment_bits; }

private:
	uptr calc_setup_size();
	cause::pair<uptr> setup_hba(table_alloc* alloc);
	cause::pair<uptr> setup_hba_cmd_list(table_alloc* alloc);
	cause::pair<uptr> setup_hba_rx_fis(table_alloc* alloc);
	cause::pair<uptr> setup_hba_cmd_table(table_alloc* alloc, int cmd_hdr);

	cause::pair<int> acquire_slot();
	void release_slot(int slot);

	cause::pair<uptr> read_atapi(uptr start, uptr bytes, void* data);
	cause::pair<uptr> read_atapi_cmd(
	    u32 seg_start, u8 seg_count, int slot);

public:
	chain_node<ahci_device> ahci_hba_chain_node;

private:
	ahci_driver*        driver;
	ahci_hba*           hba;
	HBA_MEM_REGS::PORT* hba_port_regs;
	COMMAND_HEADER*     cmd_list;
	u8*                 rx_fis;
	COMMAND_TABLE*      cmd_table[CMD_HDR_NR];

	spin_lock           cmd_table_lock[CMD_HDR_NR];

	u32  reg_sig;

	int                 hba_port;

	bool                is_atapi;
	u16                 segment_bits;

	ahci_device_io_node ion;
};

/// @brief AHCI host bus adapter
class ahci_hba : public bus_device
{
public:
	ahci_hba(
	    const char* device_name,
	    ahci_driver* _driver,
	    uptr base_address);
	~ahci_hba() {}

public:
	cause::t setup();
	cause::t unsetup();

public:
	ahci_driver* get_driver();
	volatile HBA_MEM_REGS::PORT* ref_port_regs(int port);

	bool  get_s64a() const;    // Supports 64-bit Addressing
	int   get_ncs() const;     // Number of Command Slots

private:
	cause::t load_regs();
	cause::t detect_devices();
	cause::t append_devices(int port);

public:
	chain_node<ahci_hba> ahci_driver_chain_node;

private:
	ahci_driver* driver;
	HBA_MEM_REGS* hba_regs;
	chain<ahci_device, &ahci_device::ahci_hba_chain_node> devices;

	uptr hba_mem_padr;

	u32  reg_cap;
};

class ahci_driver : public driver
{
	using ahci_hba_chain =
	      chain<ahci_hba, &ahci_hba::ahci_driver_chain_node>;

public:
	ahci_driver();
	~ahci_driver() {}

public:
	mempool* get_mp2048() { return mp2048; }

public:
	cause::t setup();
	cause::t scan();

	cause::pair<table_alloc*> acquire_table_alloc(uptr bytes);
	void release_table_alloc();

private:
	cause::t setup_ion();
	cause::t setup_mp();
	cause::t scan_pci(pci_bus_device* pci);

private:
	ahci_hba_chain hba_chain;
	table_alloc tbl_alloc;

	mempool* mp2048;

	spin_rwlock hba_chain_lock;
	spin_lock tbl_alloc_lock;

	io_node::interfaces ion_ifs;
};

}  // namespace ahci


#endif  // include guard

