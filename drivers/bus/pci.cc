/// @file  pci.cc
/// @brief PCI driver.

//  Uniqos  --  Unique Operating System
//  (C) 2014-2015 KATO Takeshi
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

#include <arch/native_ops.hh>
#include <core/device.hh>
#include <core/new_ops.hh>
#include <util/spinlock.hh>
#include <util/string.hh>

#include <core/log.hh>


cause::t setup_ahci(uptr);

//TODO:x86 io depend

namespace {

typedef u32 pci_reg_t;

enum {
	PCI_CONFIG_ADDRESS = 0x0cf8,
	PCI_CONFIG_DATA    = 0x0cfc,
};

u32 BSF_to_baseadr(u32 bus, u32 slot, u32 func) {
	return bus << 16 | slot << 11 | func << 8 | U32(0x80000000);
}
u32 baseadr_to_bus(u32 adr) {
	return (adr >> 16) & 0xff;
}
u32 baseadr_to_slot(u32 adr) {
	return (adr >> 11) & 0x1f;
}
u32 baseadr_to_func(u32 adr) {
	return (adr >> 8) & 0x7;
}

class pci_bus_device;

class pci_driver
{
public:
	pci_driver() {}
	~pci_driver() {}

public:
	cause::t setup();

private:
	pci_bus_device* dev;
};

class pci_bus_device : public bus_device
{
	class config_entry
	{
	public:
		bichain_node<config_entry> _config_chain_node;
		bichain_node<config_entry>& config_chain_node() {
			return _config_chain_node;
		}

		u32 config_adr;
		u32 config_bytes;

		u8 table8[];
	};
	typedef bibochain<config_entry, &config_entry::config_chain_node>
	        config_chain;

public:
	pci_bus_device() {}
	~pci_bus_device() {}

public:
	cause::t setup();

private:
	void lock_config();
	void unlock_config();
	pci_reg_t _read_config(u32 base, u32 reg);
	void _write_config(pci_reg_t data, u32 base, u32 reg);
	cause::pair<config_entry*> load_config(u32 bus, u32 slot, u32 func);

private:
	config_chain configs;
	spin_lock config_lock;
};

struct config_header
{
	u16 device_id;
	u16 vendor_id;
	u16 status;
	u16 command;
	u8 class_code;
	u8 sub_class;
	u8 prog_if;
	u8 revision_id;
	u8 bist;
	u8 header_type;
	u8 latency_timer;
	u8 cache_line_size;
};

// pci_driver

u32 pciget(u32 adr)
{
	native::outl(adr, PCI_CONFIG_ADDRESS);
	return native::inl(PCI_CONFIG_DATA);
}

void pcidump(u32 bus, u32 slot, u32 func)
{
	const u32 adr = bus << 16 | slot << 11 | func << 8 | U32(0x80000000);
	u32 d[16];
	d[0] = pciget(adr);
	if ((d[0] & 0xffff) == 0xffff)
		return;

	for (int i = 1; i < 16; ++i)
		d[i] = pciget(adr + 4*i);

	log()("-- bus ").u(bus)("  slot ").u(slot)("  func").u(func)();
	log()
	("dev id:").x(d[0] >> 16, 4)("  vendor:").x(d[0] & 0xffff, 4)
	("  status:").x(d[1] >> 16, 4)("  commad:").x(d[1] & 0xffff, 4)()
	(" class:").x(d[2] >> 24, 4)("  subcla:").x((d[2] >> 16) & 0xff, 4)
	("  progif:").x((d[2]>>8)&0xff, 4)("  reviid:").x(d[2] & 0xff, 4)()
	("  bist:").x(d[3] >> 24, 4)("  header:").x((d[3] >> 16) & 0xff, 4)
	("  lattim:").x((d[3]>>8)&0xff, 4)("  cachel:").x(d[3] & 0xff, 4)();

	if ((d[3] & 0xff0000) == 0x000000) {
		log()
		(" BAR#0:").x(d[4], 8)("           BAR#1:").x(d[5], 8)()
		(" BAR#2:").x(d[6], 8)("           BAR#3:").x(d[7], 8)()
		(" BAR#4:").x(d[8], 8)("           BAR#5:").x(d[9], 8)();
	}

	if ((d[2] & 0xffff0000) == 0x01060000) {
		setup_ahci(d[9]);
	}
}

cause::t pci_driver::setup()
{
	for (u32 bus = 0; bus < 256; ++bus) {
		for (u32 slot = 0; slot < 32; ++slot) {
			for (u32 func = 0; func < 8; ++func) {
				pcidump(bus, slot, func);
			}
		}
	}

	pci_bus_device* d = new (generic_mem()) pci_bus_device;
	if (!d)
		return cause::NOMEM;

	cause::t r = d->setup();
	if (is_fail(r))
		return r;

	return cause::OK;
}

// pci_bus_device

cause::t pci_bus_device::setup()
{
	spin_lock_section sls(config_lock);

	for (u32 bus = 0; bus < 256; ++bus) {
		for (u32 slot = 0; slot < 32; ++slot) {
			auto entry = load_config(bus, slot, 0);
			if (!entry)
				continue;
			config_header* header =
			    reinterpret_cast<config_header*>
			    (entry.data()->table8);
			if (!(header->header_type & 0x80))
				continue;
			for (u32 func = 1; func < 8; ++func)
				load_config(bus, slot, func);
		}
	}

	return cause::OK;
}

void pci_bus_device::lock_config()
{
	config_lock.lock();
}

void pci_bus_device::unlock_config()
{
	config_lock.unlock();
}

/// No lock.
pci_reg_t pci_bus_device::_read_config(u32 base, u32 reg)
{
	native::outl(base + reg * sizeof (pci_reg_t), PCI_CONFIG_ADDRESS);

	return native::inl(PCI_CONFIG_DATA);
}

/// No lock.
void pci_bus_device::_write_config(u32 data, u32 base, u32 reg)
{
	native::outl(base + reg * sizeof (pci_reg_t), PCI_CONFIG_ADDRESS);

	return native::outl(data, PCI_CONFIG_DATA);
}

cause::pair<pci_bus_device::config_entry*> pci_bus_device::load_config(
    u32 bus, u32 slot, u32 func)
{
	u32 base = BSF_to_baseadr(bus, slot, func);

	pci_reg_t header[4];

	header[0] = _read_config(base, 0);
	if ((header[0] & 0xffff) == 0xffff) {
		// No device
		return null_pair(cause::OK);
	}

	for (uint reg = 1; reg < num_of_array(header); ++reg)
		header[reg] = _read_config(base, reg);

	uptr table_bytes;
	switch((header[3] >> 16) & 0x7f)  // header type
	{
	case 0x00: table_bytes = 0x40; break;
	case 0x01: table_bytes = 0x40; break;
	case 0x02: table_bytes = 0x48; break;
	default:   // unsupported
		return cause::null_pair(cause::FAIL);
	}

	auto mem = generic_mem().allocate(sizeof (config_entry) + table_bytes);
	if (is_fail(mem))
		return null_pair(mem.cause());

	config_entry* ce = new (mem.data()) config_entry;
	pci_reg_t* table = reinterpret_cast<pci_reg_t*>(ce->table8);
	mem_copy(header, table, sizeof header);

	const uint n = table_bytes / sizeof (pci_reg_t);
	for (uint reg = num_of_array(header); reg < n; ++reg)
		table[reg] = _read_config(base, reg);

	ce->config_adr = base;
	ce->config_bytes = table_bytes;

	configs.push_back(ce);

	return make_pair(cause::OK, ce);
}

}

cause::t pci_setup()
{
	pci_driver* drv = new (generic_mem()) pci_driver;
	if (!drv)
		return cause::NOMEM;

	cause::t r = drv->setup();
	if (is_fail(r))
		return r;

	return r;
}

cause::t pci_unsetup()
{
	return cause::NOFUNC;
}

