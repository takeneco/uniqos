/// @file  ahci.cc
/// @brief AHCI driver.

//  Uniqos  --  Unique Operating System
//  (C) 2015 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "ahci.hh"

#include <arch.hh>
#include <core/global_vars.hh>
#include <core/int_bitset.hh>
#include <core/log.hh>
#include <core/new_ops.hh>
#include <core/page.hh>
#include <core/vadr_pool.hh>
#include <util/string.hh>


using namespace ahci;

namespace {

enum GHC_FLAGS {
	GHC_AE = U32(1) << 31,   ///< AHCI Enable
};

}  // namespace

namespace {
class ahci_driver; // for friend declaration.
}

namespace {

const char driver_name_ahci[] = "ahci";
const char device_name_ahci_hba[] = "ahci-hba";
const char device_name_ahci_dev[] = "ahci-dev";

}  // namespace

namespace ahci {

// ahci_device

ahci_device::ahci_device(ahci_hba* _hba, int port) :
	device(device_name_ahci_dev),
	hba(_hba),
	hba_port_regs(_hba->ref_port_regs(port)),
	cmd_list(nullptr),
	rx_fis(nullptr),
	hba_port(port)
{
	for (uint i = 0; i < CMD_HDR_NR; ++i)
		cmd_table[i] = nullptr;
}

ahci_device::~ahci_device()
{
}

cause::t ahci_device::setup()
{
	reg_sig = hba_port_regs->sig;

	cause::t r = stop();
	if (is_fail(r))
		return r;

	uptr bytes = calc_setup_size();

	for (;;) {
		ahci_driver* drv = hba->get_driver();
		auto r2 = drv->acquire_table_alloc(bytes);
		if (is_fail(r2))
			return r2.cause();

		table_alloc* tbl_alloc = r2.data();
		cause::pair<uptr> r3 = setup_hba(tbl_alloc);

		// If r3.cause() == NOMEM, r3.data() indicates required
		// memory size in tbl_alloc.

		drv->release_table_alloc();

		if (r3.cause() == cause::NOMEM)
			bytes = r3.data();
		else if (is_ok(r3))
			break;
		else
			return r3.cause();
	}

	r = start();
	if (is_fail(r))
		return r;

	if (reg_sig == 0xeb140101) { // atapi
		void* mem = new (generic_mem()) char[4096];
		mem_fill(0xff, mem, 4096);
		auto r2 = _read(0, mem, 4096);
		log()("--- read ---r2:").u(r2.cause())();
		log().x(4096, mem, 1, 16)();
		generic_mem().deallocate(mem);
	}

	return cause::OK;
}

cause::t ahci_device::start()
{
	while (hba_port_regs->cmd & PxCMD_CR);

	hba_port_regs->cmd |= PxCMD_FRE;

	hba_port_regs->cmd |= PxCMD_ST;

	return cause::OK;
}

cause::t ahci_device::stop()
{
	hba_port_regs->cmd &= ~PxCMD_ST;
	while (hba_port_regs->cmd & PxCMD_CR);

	hba_port_regs->cmd &= ~PxCMD_FRE;
	while (hba_port_regs->cmd & PxCMD_FR);

	return cause::OK;
}

cause::pair<uptr> ahci_device::_read(uptr seg, void* data, uptr bytes)
{
	auto _slot = acquire_slot();
	if (is_fail(_slot)) {
#warning no impl:acquire_slot() failed.
		return zero_pair(_slot.cause());
	}
	int slot = _slot.data();

	hba_port_regs->is = 0xffffffff;

	COMMAND_HEADER* cmdhdr = &cmd_list[slot];
	COMMAND_TABLE* cmdtbl = cmd_table[slot];

	cmdhdr->cfl = sizeof (FIS_H2D) / sizeof (u32);
	cmdhdr->w = 0;
	cmdhdr->prdtl = 1;

	uptr dba = arch::unmap_phys_adr(data, bytes);
	cmdtbl->prdt[0].dba = (u32)dba;
	cmdtbl->prdt[0].dbau = (u32)(dba >> 32);
	cmdtbl->prdt[0].dbc = (bytes - 1) | 0x1;
	cmdtbl->prdt[0].i = 1;

	FIS_H2D* cmdfis = reinterpret_cast<FIS_H2D*>(cmdtbl->cfis);
	cmdfis->fis_type = 0x27;  // FIS_TYPE_REG_H2D
	cmdfis->c = 1;  // Command
	// IF AHCI
	// cmdfis->command = 0x25;  // ATA_CMD_READ_DMA_EX
	// IF ATAPI
	cmdhdr->a = 1;
	cmdfis->command = 0xa0;  // ATA_PACKET
	//
	cmdfis->lba0 = (u8)seg;
	cmdfis->lba1 = (u8)(seg >> 8);
	cmdfis->lba2 = (u8)(seg >> 16);
	cmdfis->device = 1<<6; // LBA device
	cmdfis->lba3 = (u8)(seg >> 24);
	cmdfis->lba4 = (u8)(seg >> 32);
	cmdfis->lba5 = (u8)(seg >> 40);

	cmdfis->countl = bytes >> 8;
	cmdfis->counth = bytes >> 16;

	//IF ATAPI
	cmdtbl->acmd[ 0] = 0xa8;
	cmdtbl->acmd[ 1] = 0;
	cmdtbl->acmd[ 2] = 0;
	cmdtbl->acmd[ 3] = 0;
	cmdtbl->acmd[ 4] = 0;
	cmdtbl->acmd[ 5] = 0100000 / 2048;
	cmdtbl->acmd[ 6] = 0;
	cmdtbl->acmd[ 7] = 0;
	cmdtbl->acmd[ 8] = 0;
	cmdtbl->acmd[ 9] = 2;
	cmdtbl->acmd[10] = 0;
	cmdtbl->acmd[11] = 0;
	//

	enum {
		ATA_DEV_BUSY = 0x80,
		ATA_DEV_DRQ = 0x80,
	};
	int spin;
	for (spin = 0;
	     spin < 1000000 && (hba_port_regs->tfd & (ATA_DEV_BUSY|ATA_DEV_DRQ));
	     ++spin)
	{
	}
	if (spin == 1000000) {
		log()("Port is hung")();
		return zero_pair(cause::FAIL);
	}

	hba_port_regs->ci = 1 << 0;

	enum {
		HBA_PxIS_TFES = 0x40000000,
	};
	for (int i = 0; ; ++i) {
		if ((hba_port_regs->ci & (1 << 0)) == 0)
			break;
		if (hba_port_regs->is & HBA_PxIS_TFES) {
			log()("Read disk error|is = ").x(hba_port_regs->is)();
			return zero_pair(cause::FAIL);
		}
	}

	return cause::pair<uptr>(cause::OK, 0);
}

/// setup()によって割り当てられるAHCIレジスタ用のメモリサイズを計算する。
uptr ahci_device::calc_setup_size()
{
	uptr bytes = 0;

	const int ncs = hba->get_ncs();

	bytes += sizeof (COMMAND_HEADER) * ncs;

	bytes += RX_FIS_SZ;

	bytes += (
	       sizeof (COMMAND_TABLE) + sizeof (COMMAND_TABLE::PRDT) * PRDT_NR
	       ) * ncs;

	return bytes;
}

/// @brief  Allocate memory to HBA.
/// @return  この関数がNOMEMを返したときは、table_allocへメモリを追加して
///  再び呼び出せば成功する。必要なメモリサイズも返す。
///  十分なメモリがあってもハードウェアの機能不足により失敗することがある。
cause::pair<uptr> ahci_device::setup_hba(table_alloc* alloc)
{
	cause::pair<uptr> r;

	r = setup_hba_cmd_list(alloc);
	if (is_fail(r))
		return r;

	r = setup_hba_rx_fis(alloc);
	if (is_fail(r))
		return r;

	const int ncs = hba->get_ncs();
	for (int i = 0; i < ncs; ++i) {
		r = setup_hba_cmd_table(alloc, i);
		if (is_fail(r))
			return r;
	}

	return zero_pair(cause::OK);
}

cause::pair<uptr> ahci_device::setup_hba_cmd_list(table_alloc* alloc)
{
	if (cmd_list)
		return zero_pair(cause::OK);

	uptr size = sizeof (COMMAND_HEADER) * hba->get_ncs();

	void* padr = alloc->alloc(TABLE_ALLOC_SLOT_MASK,
	                          size,
	                          CMD_HDR_ALIGN, true);
	if (!padr)
		return make_pair(cause::NOMEM, size);

	void* vadr = arch::map_phys_adr(padr, size);

	u32 padr_l = reinterpret_cast<uptr>(padr);
	u32 padr_h = reinterpret_cast<uptr>(padr) >> 32;

	hba_port_regs->clb = padr_l;

	if (hba->get_s64a()) {
		hba_port_regs->clbu = padr_h;
	} else if (padr_h != 0) {
		log()(SRCPOS)(":64bit addressing not supported")();
		return zero_pair(cause::FAIL);
	}

	mem_fill(0, vadr, size);

	cmd_list = static_cast<COMMAND_HEADER*>(vadr);

	return zero_pair(cause::OK);
}

cause::pair<uptr> ahci_device::setup_hba_rx_fis(table_alloc* alloc)
{
	if (rx_fis)
		return zero_pair(cause::OK);

	uptr size = RX_FIS_SZ;

	void* padr = alloc->alloc(TABLE_ALLOC_SLOT_MASK,
	                          size,
	                          RX_FIS_ALIGN,
	                          true);
	if (!padr)
		return make_pair(cause::NOMEM, size);

	void* vadr = arch::map_phys_adr(padr, size);

	u32 padr_l = reinterpret_cast<uptr>(padr);
	u32 padr_h = reinterpret_cast<uptr>(padr) >> 32;

	hba_port_regs->fb = padr_l;

	if (hba->get_s64a()) {
		hba_port_regs->fbu = padr_h;
	} else if (padr_h != 0) {
		log()(SRCPOS)(":64bit addressing not supported")();
		return zero_pair(cause::FAIL);
	}

	mem_fill(0, vadr, size);

	rx_fis = static_cast<u8*>(vadr);

	return zero_pair(cause::OK);
}

cause::pair<uptr> ahci_device::setup_hba_cmd_table(
    table_alloc* alloc, int cmd_hdr)
{
	if (cmd_table[cmd_hdr])
		return zero_pair(cause::OK);

	uptr size = sizeof (COMMAND_TABLE)
	          + sizeof (COMMAND_TABLE::PRDT) * PRDT_NR;

	void* padr = alloc->alloc(TABLE_ALLOC_SLOT_MASK,
	                          size,
	                          CMD_TBL_ALIGN,
	                          true);
	if (!padr)
		return make_pair(cause::NOMEM, size);

	void* vadr = arch::map_phys_adr(padr, size);

	u32 padr_l = reinterpret_cast<uptr>(padr);
	u32 padr_h = reinterpret_cast<uptr>(padr) >> 32;

	cmd_list[cmd_hdr].ctba = padr_l;

	if (hba->get_s64a()) {
		cmd_list[cmd_hdr].ctbau0 = padr_h;
	} else if (padr_h != 0) {
		log()(SRCPOS)(":64bit addressing not supported")();
		return zero_pair(cause::FAIL);
	}

	mem_fill(0, vadr, size);

	cmd_table[cmd_hdr] = static_cast<COMMAND_TABLE*>(vadr);

	return zero_pair(cause::OK);
}

cause::pair<int> ahci_device::acquire_slot()
{
	int_bitset<u32> slots(hba_port_regs->sact | hba_port_regs->ci);

	// slotsの各ビットがcmd_tableに対応する。
	// ビットが0なら未使用だが、cmd_table_lockをロックできなければ、
	// そのcmd_tableを使おうとしているスレッドが他にいる。

	for (;;) {
		int slot = slots.search_false();
		if (slot < 0) {
			log()(SRCPOS)(":no command slots.");
			return zero_pair(cause::NODEV);
		}

		if (cmd_table_lock[slot].try_lock_np())
			return make_pair(cause::OK, slot);
		else
			slots.set_true(slot);
	}

}

void ahci_device::release_slot(int slot)
{
	cmd_table_lock[slot].unlock_np();
}


// ahci_hba

ahci_hba::ahci_hba(ahci_driver* _driver, uptr base_address) :
	bus_device(device_name_ahci_hba),
	driver(_driver),
	hba_mem_padr(base_address)
{
}

cause::t ahci_hba::setup()
{
	vadr_pool* pool = global_vars::core.vadr_pool_obj;
	auto mem = pool->assign(hba_mem_padr, sizeof *hba_regs,
	    PAGE_CACHE_DISABLE |
	    PAGE_WRITE_THROUGH |
	    PAGE_DENY_USER |
	    PAGE_ACCESSED |
	    PAGE_DIRTIED |
	    PAGE_GLOBAL);
	if (is_fail(mem))
		return mem.cause();

	hba_regs = static_cast<HBA_MEM_REGS*>(mem.data());

	// Initialization of HBA

	// enable ahci
	hba_regs->ghc |= GHC_AE;

	cause::t r = load_regs();
	if (is_fail(r))
		return r;

	r = detect_devices();
	if (is_fail(r))
		return r;

	for (ahci_device* dev : devices) {
		// TODO: r
		r = dev->setup();
	}

	return cause::OK;
}

cause::t ahci_hba::unsetup()
{
	return cause::NOFUNC;
}

ahci_driver* ahci_hba::get_driver()
{
	return driver;
}

volatile HBA_MEM_REGS::PORT* ahci_hba::ref_port_regs(int port)
{
	return &hba_regs->port[port];
}

bool ahci_hba::get_s64a() const {
	return (hba_regs->cap & CAP_S64A) != 0;
}

int ahci_hba::get_ncs() const {
	return ((reg_cap & CAP_NCS_MASK) >> CAP_NCS_SHIFT) + 1;
}

cause::t ahci_hba::load_regs()
{
	reg_cap = hba_regs->cap;

	return cause::OK;
}

cause::t ahci_hba::detect_devices()
{
	// detect implemented ports
	u32 ports = hba_regs->pi;
	for (int i = 0; ports; ++i) {
		if (ports & 1) {
			auto port = &hba_regs->port[i];
			u32 ssts = port->ssts;
			if (ssts & 0x00000f00) {
				append_devices(i);
			}
			/*
			if (port->sig == 0x101) {
				// ahci
			} else if (port->sig == 0xeb140101) {
				// atapi
			}
			*/
		}
		ports >>= 1;
	}

	return cause::OK;
}

cause::t ahci_hba::append_devices(int port)
{
	ahci_device* dev = new (generic_mem()) ahci_device(this, port);

	devices.push_back(dev);

	/*
	cause::t r = get_device_ctl()->append_device(dev);
	if (is_fail(r))
		log()("!!! r=").u(r)();
	*/

	return cause::OK;
}

// ahci_driver

ahci_driver::ahci_driver() :
	driver(driver_name_ahci)
{
}

cause::t ahci_driver::setup()
{
	return cause::OK;
}

cause::t ahci_driver::scan()
{
	device_ctl* devctl = get_device_ctl();
	if (!devctl)
		return cause::FAIL;

	for (auto bus : devctl->each_bus_devices()) {
		const sint comp = str_compare(
		    device_name_pci, bus->get_name(), device::NAME_NR);
		if (comp == 0) {
			pci_bus_device* pci = static_cast<pci_bus_device*>(bus);
			cause::t r = scan_pci(pci);
			if (is_fail(r))
				return r;
		}
	}

	for (ahci_hba* hba : hba_chain) {
		cause::t r = get_device_ctl()->append_bus_device(hba);
		if (is_fail(r))
			log()(SRCPOS)("!!! r=").u(r)();

		// TODO:page_poolからbase_addressを除外しなくてよいか？

		r = hba->setup();
		if (is_fail(r))
			log()("ahci_hba::setup() failed. r=").u(r)();
	}

	return cause::OK;
}

auto ahci_driver::acquire_table_alloc(uptr bytes)
-> cause::pair<table_alloc*>
{
	sptr table_chunk_bytes = sizeof (COMMAND_HEADER) * CMD_HDR_NR;

	page_level pg_lv = page_level_of_size(table_chunk_bytes);
	uptr pg_sz = page_size_of_level(pg_lv);

	tbl_alloc_lock.lock();

	uptr free_bytes = tbl_alloc.total_free_bytes(TABLE_ALLOC_SLOT_MASK);
	sptr want_bytes = bytes - free_bytes;

	for (;;) {
		if (want_bytes <= 0)
			break;
		if (want_bytes < table_chunk_bytes) {
			pg_lv = page_level_of_size(want_bytes);
			pg_sz = page_size_of_level(pg_lv);
		}

		auto padr = page_alloc(pg_lv);
		if (is_fail(padr)) {
			tbl_alloc_lock.unlock();
			log()(SRCPOS)(":page_alloc() failed. r=").u(padr.cause())();
			return null_pair(padr.cause());
		}

		bool r = tbl_alloc.add_free(TABLE_ALLOC_SLOT, padr.data(), pg_sz);
		if (!r) {
			tbl_alloc_lock.unlock();
			log()(SRCPOS)(":add_free() failed.")();
			return null_pair(cause::FAIL);
		}

		want_bytes -= pg_sz;
	}

	return make_pair(cause::OK, &tbl_alloc);
}

void ahci_driver::release_table_alloc()
{
	tbl_alloc_lock.unlock();
}

cause::t ahci_driver::scan_pci(pci_bus_device* pci)
{
	for (pci_device* dev : pci->each_devices()) {
		if (dev->get_class().data() == PCI_CLASS_STORAGE_SATA) {
			// BAR#5 is base_address of AHCI
			auto ba = dev->get_base_address(5);
			if (is_fail(ba))
				return ba.cause();
			ahci_hba* ahci =
			    new (generic_mem()) ahci_hba(this, ba.data());
			if (!ahci)
				return cause::NOMEM;

			spin_wlock_section _sws(hba_chain_lock);
			hba_chain.push_back(ahci);
		}
	}

	return cause::OK;
}

}  // namespace ahci


void dump_ahci(int port, volatile HBA_MEM_REGS::PORT* port_reg)
{
	enum {
		HBA_PxCMD_ST  = 0x00000001,
		HBA_PxCMD_FRE = 0x00000010,
		HBA_PxCMD_FR  = U32(1) << 14,
		HBA_PxCMD_CR  = U32(1) << 15,
	};

	u32 ssts = port_reg->ssts;
	u8 ipm = (ssts >> 8) & 0x0f;
	u8 det = ssts & 0x0f;
	log()("ipm:").u(ipm)(" det:").u(det)();

	// init

	//   stop
	log()("AHCI stop|cmd = ").x(port_reg->cmd)();
	port_reg->cmd &= ~HBA_PxCMD_ST;
	uint i;
	for (i = 0; port_reg->cmd & HBA_PxCMD_CR; ++i);
	log()("i=").u(i);

	port_reg->cmd &= ~HBA_PxCMD_FRE;
	for (i = 0; port_reg->cmd & HBA_PxCMD_FR; ++i);
	log()(" i=").u(i)();

	//    rebase
	log()("AHCI rebase")();
	uptr padr;
	page_alloc(arch::page::PHYS_L1, &padr);
	log()("AHCI command list = ").x(padr)();
	COMMAND_HEADER* cl = (COMMAND_HEADER*)arch::map_phys_adr(padr, 4096);
	mem_fill(0, cl, 4096);

	port_reg->clb = (u32)padr;
	port_reg->clbu = 0;

	page_alloc(arch::page::PHYS_L1, &padr);
	mem_fill(0, arch::map_phys_adr(padr, 4096), 4096);

	log()("AHCI port fis base = ").x(padr)();
	port_reg->fb = (u32)padr;
	port_reg->fbu = 0;

	page_alloc(arch::page::PHYS_L1, &padr);
	COMMAND_TABLE* cmdtbl = (COMMAND_TABLE*)arch::map_phys_adr(padr, 4096);
	mem_fill(0, cmdtbl, 4096);
	log()("AHCI command table = ").x(padr)();
	cl->prdtl = 8;
	cl->ctba = padr;
	cl->ctbau0 = 0;

	//    start
	log()("AHCI start")();
	while (port_reg->cmd & HBA_PxCMD_CR);

	port_reg->cmd |= HBA_PxCMD_FRE;
	port_reg->cmd |= HBA_PxCMD_ST;

	//read
	u32 slots = port_reg->sact | port_reg->ci;
	int slot = 0;
	for (int i = 0; slots; ++i) {
		if ((slots & 1) == 0)
			slot = i;
		slots >>= 1;
	}
	log()("Port slot = ").u(slot)();

	FIS_H2D* cmdfis = (FIS_H2D*)&cmdtbl->cfis;

	port_reg->is = 0xffffffff;

	cl->cfl = sizeof (FIS_H2D) / sizeof (u32);
	cl->w = 0;
	cl->prdtl =  1;

	page_alloc(arch::page::PHYS_L1, &padr);
	u8* buf = (u8*)arch::map_phys_adr(padr, 4096);
	mem_fill(0xff, buf, 4096);
	log()("AHCI buf = ").x(padr)();
	cmdtbl->prdt[0].dba = padr; //buf;
	cmdtbl->prdt[0].dbc = 4 * 1024;
	cmdtbl->prdt[0].i = 1; // intr completion

	cmdfis->fis_type = 0x27;  // FIS_TYPE_REG_H2D
	cmdfis->c = 1; // Command
	cmdfis->command = 0x25; // ATA_CMD_READ_DMA_EX;

	cmdfis->lba0 = 1;
	cmdfis->lba1 = 0;
	cmdfis->lba2 = 0;
	cmdfis->device = 1<<6;//LBA mode
	cmdfis->lba3 = 0;
	cmdfis->lba4 = 0;
	cmdfis->lba5 = 0;

	cmdfis->countl = 8;
	cmdfis->counth = 0;

	enum {
		ATA_DEV_BUSY = 0x80,
		ATA_DEV_DRQ = 0x08
	};
	int spin;
	for (spin = 0;
	     spin < 1000000 && (port_reg->tfd & (ATA_DEV_BUSY|ATA_DEV_DRQ));
	     ++spin)
	{
	}
	if (spin == 1000000) {
		log()("Port is hung")();
		return;
	}
	log()("Port is ready")();

	port_reg->ci = 1 << 0;

	enum {
		HBA_PxIS_TFES = 0x40000000,
	};
	for (int i = 0;; ++i) {
		if ((port_reg->ci & (1 << 0)) == 0)
			break;
		if (port_reg->is & HBA_PxIS_TFES) {
			log()("Read disk error|is = ").x(port_reg->is)();
			return;
		}
		if ((i & 0xffff) == 0xffff)
			log()("|ci=").x(port_reg->ci)(":is=").x(port_reg->is);
	}

	if (port_reg->is & HBA_PxIS_TFES) {
		log()("Read disk error|is = ").x(port_reg->is)();
		return;
	}

	log()("Read disk success")();
	return;
}


void dump_atapi(int port, volatile HBA_MEM_REGS::PORT* port_reg)
{
	enum {
		HBA_PxCMD_ST  = 0x00000001,
		HBA_PxCMD_FRE = 0x00000010,
		HBA_PxCMD_FR  = U32(1) << 14,
		HBA_PxCMD_CR  = U32(1) << 15,
	};

	u32 ssts = port_reg->ssts;
	u8 ipm = (ssts >> 8) & 0x0f;
	u8 det = ssts & 0x0f;
	log()("ipm:").u(ipm)(" det:").u(det)();

	// init

	//   stop
	log()("AHCI stop|cmd = ").x(port_reg->cmd)();
	port_reg->cmd &= ~HBA_PxCMD_ST;
	while (port_reg->cmd & HBA_PxCMD_CR);

	port_reg->cmd &= ~HBA_PxCMD_FRE;
	while (port_reg->cmd & HBA_PxCMD_FR);

	//    rebase
	log()("AHCI rebase")();
	uptr padr;
	page_alloc(arch::page::PHYS_L1, &padr);
	log()("AHCI command list = ").x(padr)();
	COMMAND_HEADER* cl = (COMMAND_HEADER*)arch::map_phys_adr(padr, 4096);
	mem_fill(0, cl, 4096);

	port_reg->clb = (u32)padr;
	port_reg->clbu = 0;

	page_alloc(arch::page::PHYS_L1, &padr);
	mem_fill(0, arch::map_phys_adr(padr, 4096), 4096);

	log()("AHCI port fis base = ").x(padr)();
	port_reg->fb = (u32)padr;
	port_reg->fbu = 0;

	page_alloc(arch::page::PHYS_L1, &padr);
	COMMAND_TABLE* cmdtbl = (COMMAND_TABLE*)arch::map_phys_adr(padr, 4096);
	mem_fill(0, cmdtbl, 4096);
	log()("AHCI command table = ").x(padr)();
	cl->prdtl = 8;
	cl->ctba = padr;
	cl->ctbau0 = 0;

	//    start
	log()("AHCI start")();
	while (port_reg->cmd & HBA_PxCMD_CR);

	port_reg->cmd |= HBA_PxCMD_FRE;
	port_reg->cmd |= HBA_PxCMD_ST;

	//read
	u32 slots = port_reg->sact | port_reg->ci;
	int slot = 0;
	for (int i = 0; slots; ++i) {
		if ((slots & 1) == 0)
			slot = i;
		slots >>= 1;
	}
	log()("Port slot = ").u(slot)();

	FIS_H2D* cmdfis = (FIS_H2D*)&cmdtbl->cfis;

	port_reg->is = 0xffffffff;

	cl->cfl = sizeof (FIS_H2D) / sizeof (u32);
	cl->w = 0;
	cl->prdtl =  1;

	page_alloc(arch::page::PHYS_L2, &padr);
	u8* buf = (u8*)arch::map_phys_adr(padr, 4096);
	mem_fill(0xff, buf, 2 * 1024 * 1024);
	log()("AHCI buf = ").x(padr)();
	cmdtbl->prdt[0].dba = padr; //buf;
	//cmdtbl->prdt[0].dbc = 4 * 1024;
	cmdtbl->prdt[0].dbc = 2 * 1024 * 1024;
	cmdtbl->prdt[0].i = 1; // intr completion

	cmdfis->fis_type = 0x27;  // FIS_TYPE_REG_H2D
	cmdfis->c = 1; // Command
	// IF AHCI
	//cmdfis->command = 0x25; // ATA_CMD_READ_DMA_EX;
	// IF ATAPI
	cl->a = 1;
	cmdfis->command = 0xa0; // ATA_PACKET
	//

	cmdfis->lba0 = 1;
	cmdfis->lba1 = 0;
	cmdfis->lba2 = 0;
	cmdfis->device = 1<<6;//LBA mode
	cmdfis->lba3 = 0;
	cmdfis->lba4 = 0;
	cmdfis->lba5 = 0;

	cmdfis->countl = 8;
	cmdfis->counth = 0;

	// IF ATAPI
	cmdtbl->acmd[ 0] = 0xa8;
	cmdtbl->acmd[ 1] = 0;
	cmdtbl->acmd[ 2] = 0;
	cmdtbl->acmd[ 3] = 0;
	cmdtbl->acmd[ 4] = 0;
	cmdtbl->acmd[ 5] = 0;
	cmdtbl->acmd[ 6] = 0;
	cmdtbl->acmd[ 7] = 0;
	cmdtbl->acmd[ 8] = 0;
	cmdtbl->acmd[ 9] = 100;
	cmdtbl->acmd[10] = 0;
	cmdtbl->acmd[11] = 0;
	//

	enum {
		ATA_DEV_BUSY = 0x80,
		ATA_DEV_DRQ = 0x08
	};
	int spin;
	for (spin = 0;
	     spin < 1000000 && (port_reg->tfd & (ATA_DEV_BUSY|ATA_DEV_DRQ));
	     ++spin)
	{
	}
	if (spin == 1000000) {
		log()("Port is hung")();
		return;
	}
	log()("Port is ready")();

	port_reg->ci = 1 << 0;

	enum {
		HBA_PxIS_TFES = 0x40000000,
	};
	for (int i = 0;; ++i) {
		if ((port_reg->ci & (1 << 0)) == 0)
			break;
		if (port_reg->is & HBA_PxIS_TFES) {
			log()("Read disk error|is = ").x(port_reg->is)();
			return;
		}
		if ((i & 0xffff) == 0xffff)
			log()("|ci=").x(port_reg->ci)(":is=").x(port_reg->is);
	}

	if (port_reg->is & HBA_PxIS_TFES) {
		log()("Read disk error|is = ").x(port_reg->is)();
		return;
	}

	log()("Read disk success")();
	return;
}

cause::t setup_ahci_(uptr base_address)
{
	HBA_MEM_REGS* hba_mem = (HBA_MEM_REGS*)arch::map_phys_adr(
	    base_address, sizeof (HBA_MEM_REGS));

	log()("---- AHCI ----")()
	("   cap:").x(hba_mem->cap, 8)
	("    ghc:").x(hba_mem->ghc, 8)
	("     is:").x(hba_mem->is, 8)
	("     pi:").x(hba_mem->pi, 8)()
	("    vs:").x(hba_mem->vs, 8)
	(" cccctl:").x(hba_mem->ccc_ctl, 8)
	(" cccpts:").x(hba_mem->ccc_ports, 8)
	("  emloc:").x(hba_mem->em_loc, 8)()
	(" emctl:").x(hba_mem->em_ctl, 8)
	("   cap2:").x(hba_mem->cap2, 8)
	("   bohc:").x(hba_mem->bohc, 8)()
	;

	u32 pi = hba_mem->pi;

	for (int i = 0; pi; ++i) {
		if (pi & 1) {
			auto port = &hba_mem->port[i];
			log()("port:").u(i)(" sig:").x(port->sig)();
			if (port->sig == 0x101) {
				dump_ahci(i, port);
			//} else if (port->sig == 0xeb140101) {
			} else if ((port->sig&0xffff0000) == 0xeb140000) {
				dump_atapi(i, port);
			}
		}
		pi >>= 1;
	}

	return cause::OK;
}

cause::t ahci_setup()
{
	log()("sizeof(ahci_driver)=").u(sizeof (ahci::ahci_driver))();
	ahci::ahci_driver* drv = new (generic_mem()) ahci::ahci_driver;
	if (!drv)
		return cause::NOMEM;

	cause::t r = get_driver_ctl()->append_driver(drv);
	if (is_fail(r)) {
		new_destroy(drv, generic_mem());
		return r;
	}

	r = drv->setup();
	if (is_fail(r))
		return r;

	r = drv->scan();
	if (is_fail(r)) {
		log()("ahci_driver::scan() failed.")();
		return r;
	}

	return r;
}

