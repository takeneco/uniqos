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
#include <core/mem_io.hh>
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

const char driver_name_ahci[] = "ahci";
const char device_name_ahci_dev[] = "ahci-dev";

}  // namespace

namespace ahci {

// ahci_device_io_node

ahci_device_io_node::ahci_device_io_node(ahci_device* owner) :
	dev(owner)
{}

cause::pair<uptr> ahci_device_io_node::on_Write(
    offset off,
    const void* data,
    uptr bytes)
{
}

// ahci_device

ahci_device::ahci_device(
    ahci_driver* ahcidriver,
    ahci_hba* ahcihba,
    int hbaport) :
	block_device(device_name_ahci_dev),
	driver(ahcidriver),
	hba(ahcihba),
	hba_port_regs(ahcihba->ref_port_regs(hbaport)),
	cmd_list(nullptr),
	rx_fis(nullptr),
	hba_port(hbaport),
	ion(this)
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

		table_alloc* tbl_alloc = r2.value();
		cause::pair<uptr> r3 = setup_hba(tbl_alloc);

		// If r3.cause() == NOMEM, r3.value() indicates required
		// memory size in tbl_alloc.

		drv->release_table_alloc();

		if (r3.cause() == cause::NOMEM)
			bytes = r3.value();
		else if (is_ok(r3))
			break;
		else
			return r3.cause();
	}

	r = start();
	if (is_fail(r))
		return r;

	if (reg_sig == 0xeb140101) { // atapi
		uint sz = 4096;
		void* mem = new (generic_mem()) char[sz];
		mem_fill(0xff, mem, sz);
		auto r2 = read_atapi(0x8001, sz, mem);
		log()("--- read ---r2:").u(r2.cause())();
		log().x(sz, mem, 1, 16)();
		generic_mem().deallocate(mem);

		is_atapi = true;
	} else {
		is_atapi = false;
	}

	if (is_atapi)
		segment_bits = 11;
	else
		segment_bits = 9;

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

/// ATAPI の read コマンドを1回発行する。
cause::pair<uptr> ahci_device::read_atapi(
    u64 start,
    u64 bytes,
    void* data)  ///< data は物理メモリ上の連続領域でなければならない。
{
	const u16 SEG_SHIFT = 11;
	const u16 SEG_BYTES = 1 << SEG_SHIFT; // = 2048;

	auto _slot = acquire_slot();
	if (is_fail(_slot)) {
		// TODO: acquire_slot() が失敗したら成功するまで待つ。
		return zero_pair(_slot.cause());
	}
	int slot = _slot.value();

	// このフラグは割り込みの直後に確認してクリア(-1)すべき。
	hba_port_regs->is = 0xffffffff;

	COMMAND_HEADER* cmdhdr = &cmd_list[slot];
	COMMAND_TABLE* cmdtbl = cmd_table[slot];

	cmdhdr->cfl = sizeof (FIS_H2D) / sizeof (u32);
	cmdhdr->w = 0;

	u16 prdtl = 0;

	const u64 start1 = down_align<u64>(start, SEG_BYTES);
	const u64 end1 = up_align<u64>(start, SEG_BYTES);
	const u64 start2 = end1;
	const u64 end2 = down_align<u64>(start + bytes, SEG_BYTES);
	const u64 start3 = end2;
	const u64 end3 = up_align<u64>(start + bytes, SEG_BYTES);

	u32 seg_start = start1 >> SEG_SHIFT;
	u64 seg_cnt = (end3 - start1) >> SEG_SHIFT;

	u8* buf1 = nullptr;
	if (start1 != end1) {
		// 開始アドレスがセグメント境界ではないため、開始アドレスを
		// 含むセグメントを別のバッファへ読み込む。
		auto buf = driver->get_mp2048()->acquire();
		if (is_fail(buf))
			return cause::zero_pair(buf.cause());
		u8* start1_buf = static_cast<u8*>(buf.value());
		uptr start1_buf_padr =
		    arch::unmap_phys_adr(start1_buf, SEG_BYTES);
		cmdtbl->prdt[prdtl].dba =
		     static_cast<u32>(start1_buf_padr & 0xffffffff);
		cmdtbl->prdt[prdtl].dbau =
		    static_cast<u32>((start1_buf_padr >> 32) & 0xffffffff);
		cmdtbl->prdt[prdtl].dbc = (SEG_BYTES - 1) | 0x1;
		cmdtbl->prdt[prdtl].i = 1;

		buf1 = static_cast<u8*>(buf.value());
		++prdtl;
	}

	if (start2 != end2) {
		// prdtあたり4MiBしか読めないので、分割する。
		//TODO: 実は255セグメントしか読めないので510KiBしか読めない。
		//      510KiB毎にコマンドを発行する必要がある。
		u8* start2_buf = static_cast<u8*>(data) + (start2 - start);
		uptr start2_buf_padr =
		    arch::unmap_phys_adr(start2_buf, end2 - start2);
		uptr left = end2 - start2;
		while (left > 0) {
			auto size = min<u64>(left, 0x400000);
			cmdtbl->prdt[prdtl].dba = static_cast<u32>(
			    start2_buf_padr & 0xffffffff);
			cmdtbl->prdt[prdtl].dbau = static_cast<u32>(
			    (start2_buf_padr >> 32) & 0xffffffff);
			cmdtbl->prdt[prdtl].dbc = (size - 1) | 0x1;
			cmdtbl->prdt[prdtl].i = 1;

			start2_buf_padr += size;
			left -= size;

			++prdtl;
		}
	}

	u8* buf3 = nullptr;
	if (start3 != end3) {
		// 終了アドレスがセグメント境界ではないため、終了アドレスを
		// 含むセグメントを別のバッファへ読み込む。
		auto buf = driver->get_mp2048()->acquire();
		if (is_fail(buf))
			return cause::zero_pair(buf.cause());
		u8* start3_buf = static_cast<u8*>(buf.value());
		uptr start3_buf_padr =
		    arch::unmap_phys_adr(start3_buf, SEG_BYTES);
		cmdtbl->prdt[prdtl].dba =
		    static_cast<u32>(start3_buf_padr & 0xffffffff);
		cmdtbl->prdt[prdtl].dbau =
		    static_cast<u32>((start3_buf_padr >> 32) & 0xffffffff);
		cmdtbl->prdt[prdtl].dbc = (SEG_BYTES - 1) | 0x1;
		cmdtbl->prdt[prdtl].i = 1;

		buf3 = static_cast<u8*>(buf.value());
		++prdtl;
	}
	cmdhdr->prdtl = prdtl;

	auto r = read_atapi_cmd(seg_start, seg_cnt, slot);

	if (is_ok(r)) {
		u8* dest = static_cast<u8*>(data);
		if (buf1) {
			mem_copy(buf1 + (start - start1),
			         dest,
			         end1 - start);
		}
		if (buf3) {
			mem_copy(buf3,
			         dest + (start3 - start),
			         (start + bytes) - start3);
		}
	}

	if (buf1)
		driver->get_mp2048()->release(buf1);

	if (buf3)
		driver->get_mp2048()->release(buf3);

	return r;
}

/// Read コマンドを１回発行する。
/// 読み込むアドレスはセグメント単位で指定する。
/// 読み込み先として padr を設定した slot を指定する。
cause::pair<uptr> ahci_device::read_atapi_cmd(
    u32 seg_start,  ///< Start segment.
    u8  seg_count,  ///< Segment count.
    int slot)       ///< Destination slot.
{
	COMMAND_HEADER* cmdhdr = &cmd_list[slot];
	COMMAND_TABLE* cmdtbl = cmd_table[slot];
	FIS_H2D* cmdfis = reinterpret_cast<FIS_H2D*>(cmdtbl->cfis);

	cmdfis->fis_type = 0x27;  // FIS_TYPE_REG_H2D
	cmdfis->c = 1;  // Command
	cmdhdr->a = 1;  // ATAPI
	cmdfis->command = 0xa0;  // ATA_PACKET

	cmdtbl->acmd[ 0] = 0xa8;
	cmdtbl->acmd[ 1] = 0;
	cmdtbl->acmd[ 2] = static_cast<u8>((seg_start >> 24) & 0xff);
	cmdtbl->acmd[ 3] = static_cast<u8>((seg_start >> 16) & 0xff);
	cmdtbl->acmd[ 4] = static_cast<u8>((seg_start >>  8) & 0xff);
	cmdtbl->acmd[ 5] = static_cast<u8>(seg_start & 0xff);
	cmdtbl->acmd[ 6] = 0;
	cmdtbl->acmd[ 7] = 0;
	cmdtbl->acmd[ 8] = 0;
	cmdtbl->acmd[ 9] = static_cast<u8>(seg_count);
	cmdtbl->acmd[10] = 0;
	cmdtbl->acmd[11] = 0;


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

	hba_port_regs->ci = 1 << slot;

	enum {
		HBA_PxIS_TFES = 0x40000000,
	};
	for (int i = 0; ; ++i) {
		if ((hba_port_regs->ci & (1 << slot)) == 0)
			break;
		if (hba_port_regs->is & HBA_PxIS_TFES) {
			log()("Read disk error|is = ").x(hba_port_regs->is)();
			return zero_pair(cause::FAIL);
		}
	}

	return cause::pair<uptr>(cause::OK, 0);
}


// ahci_hba

ahci_hba::ahci_hba(
    const char* device_name,
    ahci_driver* _driver,
    uptr base_address) :
	bus_device(device_name),
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

	hba_regs = static_cast<HBA_MEM_REGS*>(mem.value());

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
	ahci_device* dev = new (generic_mem()) ahci_device(driver, this, port);

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
	driver(driver_name_ahci),
	mp2048(nullptr)
{
}

cause::t ahci_driver::setup()
{
	auto r = setup_ion();
	if (is_fail(r))
		return r;

	r = setup_mp();
	if (is_fail(r))
		return r;

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
		cause::t r = get_device_ctl()->append_device(hba);
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

		bool r = tbl_alloc.add_free(
		    TABLE_ALLOC_SLOT, padr.value(), pg_sz);
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

cause::t ahci_driver::setup_ion()
{
	ion_ifs.init();

	return cause::OK;
}

cause::t ahci_driver::setup_mp()
{
	auto mp = mempool::acquire_shared(2048);
	if (is_fail(mp))
		return mp.cause();
	mp2048 = mp.value();

	return cause::OK;
}

cause::t ahci_driver::scan_pci(pci_bus_device* pci)
{
	for (pci_device* dev : pci->each_devices()) {
		if (dev->get_class().value() == PCI_CLASS_STORAGE_SATA) {
			// BAR#5 is base_address of AHCI
			auto ba = dev->get_base_address(5);
			if (is_fail(ba))
				return ba.cause();
			char name[device::NAME_NR];
			mem_io name_io(name);
			output_buffer name_buf(&name_io, 0);
			pci_bsf bsf;
			dev->get_bsf(&bsf);
			name_buf("ahci-hba").
			    x(bsf.pci.bus, 2).
			    x(bsf.pci.slot, 2).
			    x(bsf.pci.func, 1);
			ahci_hba* ahci = new (generic_mem())
			     ahci_hba(name, this, ba.value());
			if (!ahci)
				return cause::NOMEM;

			spin_wlock_section _sws(hba_chain_lock);
			hba_chain.push_back(ahci);
		}
	}

	return cause::OK;
}

}  // namespace ahci


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

