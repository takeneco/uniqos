/// @file  pci.cc
/// @brief PCI driver.

//  Uniqos  --  Unique Operating System
//  (C) 2014-2015 KATO Takeshi
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
#include <core/new_ops.hh>
#include <core/log.hh>


const char device_name_pci[] = "pci";

namespace {

constexpr char driver_name_pci[] = "pci";

}  // namespace

// pci_device::interfaces

void pci_device::interfaces::init()
{
	Read  = pci_device::nofunc_Read;
	Write = pci_device::nofunc_Write;
}

// pci_device

cause::pair<u32> pci_device::read(u16 offset, u8 bytes)
{
	if (LIKELY(
	    (bytes == 1) ||
	    (bytes == 2 && (offset & 1) == 0) ||
	    (bytes == 4 && (offset & 3) == 0)))
	{
		return ifs->Read(this, offset, bytes);
	}

	return zero_pair(cause::BADARG);
}

cause::t pci_device::write(u16 offset, u8 bytes, u32 value)
{
	if (LIKELY(
	    (bytes == 1) ||
	    (bytes == 2 && (offset & 1) == 0) ||
	    (bytes == 4 && (offset & 3) == 0)))
	{
		return ifs->Write(this, offset, bytes, value);
	}

	return cause::BADARG;
}

// pci_bus_device::interfaces

void pci_bus_device::interfaces::init()
{
	EachDevices = nullptr;
}

// pci_bus_device

pci_bus_device::pci_bus_device(interfaces* ifs) :
	bus_device(device_name_pci),
	_ifs(ifs)
{
}

pci_bus_device::~pci_bus_device()
{
}

foreach<pci_device*> pci_bus_device::each_devices()
{
	return _ifs->EachDevices(this);
}

// pci_driver

pci_driver::pci_driver() :
	driver(driver_name_pci)
{
}

cause::t pci_driver::setup()
{
	init_ifs();

	auto mp = mempool::acquire_shared(sizeof (pci3_device));
	if (is_fail(mp))
		return mp.cause();
	pci3_device_mp = mp.data();

	cause::t r = setup_by_acpi();
	if (is_ok(r) || r != cause::NODEV)
		return r;

	return r;
}

cause::pair<pci3_device*> pci_driver::create_pci3_device(void* pci_config_space)
{
	pci3_device* dev = new (*pci3_device_mp) pci3_device(
	    &pci3_device_ifs, pci_config_space);
	if (!dev)
		return null_pair(cause::NOMEM);

	return make_pair(cause::OK, dev);
}

void pci_driver::init_ifs()
{
	pci3_bus_device::init_ifs(
	    &pci3_bus_device_ifs,
	    &pci3_bus_device_each_devices_ifs);

	pci3_device::init_ifs(&pci3_device_ifs);
}

cause::t pci_driver::setup_by_acpi()
{
#if CONFIG_ACPI
	ACPI_TABLE_MCFG* acpi_mcfg;
	char sig[] = ACPI_SIG_MCFG;
	ACPI_STATUS r2 = AcpiGetTable(sig, 0,
	    reinterpret_cast<ACPI_TABLE_HEADER**>(&acpi_mcfg));
	if (ACPI_FAILURE(r2))
		return cause::NODEV;

	pci3_bus_device* d = new (generic_mem()) pci3_bus_device(
	    &pci3_bus_device_ifs,
	    &pci3_bus_device_each_devices_ifs,
	    this);
	if (!d)
		return cause::NOMEM;

	dev = d;

	cause::t r = d->setup_by_acpi(acpi_mcfg);
	if (is_fail(r)) {
		return r;
	}

	r = get_device_ctl()->append_bus_device(d);
	if (is_fail(r)) {
		new_destroy(d, generic_mem());
		return r;
	}

	return cause::OK;

#else  // CONFIG_ACPI
	return cause::NODEV;

#endif  // CONFIG_ACPI
}


cause::t pci_setup()
{
	pci_driver* drv = new (generic_mem()) pci_driver;
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

	return r;
}

cause::t pci_unsetup()
{
	return cause::NOFUNC;
}

