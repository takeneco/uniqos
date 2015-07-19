/// @file pci.hh

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

#ifndef DRIVER_BUS_PCI_HH_
#define DRIVER_BUS_PCI_HH_

#include <core/acpi_ctl.hh>
#include <core/driver.hh>
#include <core/mempool.hh>
#include <core/pci.hh>


class pci_driver;

class pci3_device : public pci_device
{
public:
	pci3_device(pci_device::interfaces* _ifs, void* pci_config_space);

	static void init_ifs(pci_device::interfaces* ops);

	void* get_pci_config_space();

	cause::pair<u32> on_Read(u16 offset, u8 bytes);
	cause::t on_Write(u16 offset, u8 bytes, u32 value);

private:
	void* config_space;
};

class pci3_bus_device : public pci_bus_device
{
	typedef u32 pci3_reg_t;

public:
	pci3_bus_device(
	    pci_bus_device::interfaces* ifs,
	    foreach<pci_device*>::interfaces* each_dev_ifs,
	    pci_driver* _owner);
	~pci3_bus_device() {}

	static void init_ifs(
	    pci_bus_device::interfaces* ifs,
	    foreach<pci_device*>::interfaces* each_dev_ifs);

public:
	cause::t setup_by_acpi(ACPI_TABLE_MCFG* acpi_mcfg);

	foreach<pci_device*> on_EachDevices();

private:
	static pci_device* each_device_Next(void* context,
	    pci_device* dev);
	static pci_device* each_device_GetData(void* context,
	    pci_device* dev);
	static bool        each_device_IsEqual(const void* context,
	    pci_device* dev1, pci_device* dev2);
	static void        each_device_Dtor(void* context);

	cause::pair<pci3_device*> load_config(u8 bus, u8 slot, u8 func);

	uptr get_mmio_adr(u32 bus, u32 dev, u32 func) const;

private:
	uptr mmio_base_adr;

	foreach<pci_device*>::interfaces* each_devices_ifs;
	pci_driver* owner;
};

class pci_driver : public driver
{
public:
	pci_driver();
	~pci_driver() {}

public:
	cause::t setup();

	cause::pair<pci3_device*> create_pci3_device(void* pci_config_space);

private:
	void init_ifs();
	cause::t setup_by_acpi();

private:
	pci_bus_device* dev;

	mempool* pci3_device_mp;

	pci_bus_device::interfaces       pci3_bus_device_ifs;
	foreach<pci_device*>::interfaces pci3_bus_device_each_devices_ifs;
	pci_device::interfaces           pci3_device_ifs;
};


#endif  // DRIVER_BUS_PCI_HH_

