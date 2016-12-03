/// @file  pci3.cc
/// @brief PCI 3.0 driver.

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

#include "pci.hh"

#include <arch/native_io.hh>
#include <config.h>
#include <core/global_vars.hh>
#include <core/log.hh>
#include <core/vadr_pool.hh>
#include <util/string.hh>


enum {
	PCI_CONFIG_SPACE_SZ = 0x1000,
};

namespace {

const char pci_dev_class[] = "pci";

}  // namespace

// pci3_device

pci3_device::pci3_device(
    pci_device::interfaces* _ifs,
    const pci_bsf& bsf,
    void* pci_config_space) :
	pci_device(_ifs, bsf),
	config_space(pci_config_space)
{
}

void pci3_device::init_ifs(pci_device::interfaces* ifs)
{
	ifs->init();
	ifs->Read  = pci_device::call_on_Read<pci3_device>;
	ifs->Write = pci_device::call_on_Write<pci3_device>;
}

void* pci3_device::get_pci_config_space()
{
	return config_space;
}

cause::pair<u32> pci3_device::on_Read(u16 offset, u8 bytes)
{
	u8* config = static_cast<u8*>(config_space);

	config += offset;

	u32 data;

	if (bytes == 1)
		data = arch::read_u8(config);
	else if (bytes == 2)
		data = arch::read_u16(config);
	else // if (bytes == 4)
		data = arch::read_u32(config);

	return make_pair(cause::OK, data);
}

cause::t pci3_device::on_Write(u16 offset, u8 bytes, u32 value)
{
	u8* config = static_cast<u8*>(config_space);

	config += offset;

	if (bytes == 1)
		arch::write_u8(static_cast<u8>(value), config);
	else if (bytes == 2)
		arch::write_u16(static_cast<u16>(value), config);
	else // if (bytes == 4)
		arch::write_u32(value, config);

	return cause::OK;
}


// pci3_bus_device

pci3_bus_device::pci3_bus_device(
    pci_bus_device::interfaces* ifs,
    foreach<pci_device*>::interfaces* each_dev_ifs,
    pci_driver* _owner) :
	pci_bus_device(ifs),
	each_devices_ifs(each_dev_ifs),
	owner(_owner)
{
	dev_class = pci_dev_class;
}

void pci3_bus_device::init_ifs(
    pci_bus_device::interfaces* ifs,
    foreach<pci_device*>::interfaces* each_dev_ifs)
{
	ifs->init();
	ifs->EachDevices = pci_bus_device::call_on_EachDevices<pci3_bus_device>;

	each_dev_ifs->init();
	each_dev_ifs->Next    = each_device_Next;
	each_dev_ifs->GetData = each_device_GetData;
	each_dev_ifs->IsEqual = each_device_IsEqual;
	each_dev_ifs->Dtor    = each_device_Dtor;
}

cause::t pci3_bus_device::setup_by_acpi(ACPI_TABLE_MCFG* acpi_mcfg)
{
	ACPI_MCFG_ALLOCATION* acpi_mcfg_alloc =
	    reinterpret_cast<ACPI_MCFG_ALLOCATION*>(acpi_mcfg + 1);

	// acpi_mcfg_alloc->Addressはアライメントをまたいでるので
	// ローカルにコピーする。
	decltype(acpi_mcfg_alloc->Address) mcfg_alloc_adr;
	mem_copy(&acpi_mcfg_alloc->Address,
	         &mcfg_alloc_adr,
	         sizeof mcfg_alloc_adr);

	mmio_base_adr = mcfg_alloc_adr;

	u8 start_bus = acpi_mcfg_alloc->StartBusNumber;
	u8 end_bus = acpi_mcfg_alloc->EndBusNumber;
	for (u16 bus = start_bus; bus <= end_bus; ++bus) {
		for (u8 slot = 0; slot < 32; ++slot) {
			auto dev = load_config(bus, slot, 0);
			if (dev.cause() == cause::NODEV)
				continue;
			else if (is_fail(dev))
				return dev.cause();

			if ((dev.data()->get_header_type() & 0x80) == 0)
				continue;

			for (u8 func = 1; func < 8; ++func) {
				load_config(bus, slot, func);
			}
		}
	}

	return cause::OK;
}

foreach<pci_device*> pci3_bus_device::on_EachDevices()
{
	return foreach<pci_device*>(
	    each_devices_ifs, this, devices.front(), nullptr);
}

pci_device* pci3_bus_device::each_device_Next(void* context, pci_device* dev)
{
	auto self = static_cast<pci3_bus_device*>(context);

	return self->devices.next(dev);
}

pci_device* pci3_bus_device::each_device_GetData(void*, pci_device* dev)
{
	return dev;
}

bool pci3_bus_device::each_device_IsEqual(
    const void*, pci_device* dev1, pci_device* dev2)
{
	return dev1 == dev2;
}

void pci3_bus_device::each_device_Dtor(void* context)
{
	auto self = static_cast<pci3_bus_device*>(context);

	self->devices_lock.un_rlock();
}

cause::pair<pci3_device*>
pci3_bus_device::load_config(u8 bus, u8 slot, u8 func)
{
	uptr mmio_adr = get_mmio_adr(bus, slot, func);

	vadr_pool* pool = global_vars::core.vadr_pool_obj;
	auto mem = pool->assign(mmio_adr, PCI_CONFIG_SPACE_SZ,
	    PAGE_CACHE_DISABLE |
	    PAGE_WRITE_THROUGH |
	    PAGE_DENY_USER |
	    PAGE_ACCESSED |
	    PAGE_DIRTIED |
	    PAGE_GLOBAL);
	if (is_fail(mem))
		return null_pair(mem.cause());

	void* conf_reg = mem.data();

	u16 vendor_id;
	vendor_id = arch::read_u16(conf_reg);
	if (vendor_id == 0xffff) {
		// No device
		cause::t r = pool->revoke(conf_reg);
		if (is_fail(r))
			log()(SRCPOS)(": revoke() failed. r=").u(r)();

		return null_pair(cause::NODEV);
	}

	auto dev = owner->create_pci3_device(conf_reg,
	                                     pci_bsf(bus, slot, func));
	if (is_fail(dev))
		return dev;

	log()("BSF=").u(bus)('/').u(slot)('/').u(func)();
	log().x(256, conf_reg, -8, 4, "pci config")();

	devices.push_back(dev);

	return make_pair(cause::OK, dev);
}

uptr pci3_bus_device::get_mmio_adr(u32 bus, u32 slot, u32 func) const
{
	return mmio_base_adr + (bus << 20 | slot << 15 | func << 12);
}

