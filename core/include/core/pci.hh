/// @file core/pci.hh
/// @brief PCI bus interfaces

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

#ifndef CORE_PCI_HH_
#define CORE_PCI_HH_

#include <core/device.hh>
#include <core/spinlock.hh>
#include <util/foreach.hh>


enum PCI_CONFIG_OFFSET {
	PCI_VENDOR_ID            = 0x00,  // u16
	PCI_DEVICE_ID            = 0x02,  // u16
	PCI_COMMAND              = 0x04,  // u16
	PCI_STATUS               = 0x06,  // u16
	PCI_REVISION_ID          = 0x08,  // u8
	PCI_CLASS_PROG           = 0x09,  // u8 (Prog IF)
	PCI_CLASS                = 0x0a,  // u16
	PCI_CACHE_LINE_SIZE      = 0x0c,  // u8
	PCI_LATENCY_TIMER        = 0x0d,  // u8
	PCI_HEADER_TYPE          = 0x0e,  // u8
	PCI_BIST                 = 0x0f,  // u8
	PCI_BASE_ADDRESS_0       = 0x10,  // u32
	PCI_BASE_ADDRESS_1       = 0x14,  // u32
	PCI_BASE_ADDRESS_2       = 0x18,  // u32
	PCI_BASE_ADDRESS_3       = 0x1c,  // u32
	PCI_BASE_ADDRESS_4       = 0x20,  // u32
	PCI_BASE_ADDRESS_5       = 0x24,  // u32
	PCI_CARDBUS_CIS          = 0x28,  // u32
	PCI_SUBSYSTEM_VENDOR_ID  = 0x2c,  // u16
	PCI_SUBSYSTEM_ID         = 0x2e,  // u16
	PCI_EXPANSION_ROM        = 0x30,  // u32
	PCI_CAPABILITIES_POINTER = 0x34,  // u8
	PCI_INTERRUPT_LINE       = 0x3c,  // u8
	PCI_INTERRUPT_PIN        = 0x3d,  // u8
	PCI_MIN_GNT              = 0x3e,  // u8
	PCI_MAX_LAT              = 0x3f,  // u8
};

enum PCI_CLASS {
	PCI_CLASS_STORAGE_SATA = 0x0106,
};

/// PCIのbus, slot, funcを１つの型にまとめる。
union pci_bsf
{
	u32 bsf;

	struct path
	{
		u8 bus;
		u8 slot;
		u8 func;
		u8 reserved;
		path(u8 _bus, u8 _slot, u8 _func) :
			bus(_bus), slot(_slot), func(_func), reserved(0)
		{}
	} pci;

	pci_bsf() {}
	explicit pci_bsf(const pci_bsf& x) : bsf(x.bsf) {}
	explicit pci_bsf(u8 bus, u8 slot, u8 func) : pci(bus, slot, func) {}
};

/// device on PCI
class pci_device
{
public:
	struct interfaces
	{
		void init();

		using ReadIF = cause::pair<u32> (*)(
		    pci_device* x, u16 offset, u8 bytes);
		ReadIF Read;

		using WriteIF = cause::t (*)(
		    pci_device* x, u16 offset, u8 bytes, u32 value);
		WriteIF Write;
	};

	// Read
	template<class T> static cause::pair<u32> call_on_Read(
	    pci_device* x, u16 offset, u8 bytes) {
		return static_cast<T*>(x)->on_Read(offset, bytes);
	}
	static cause::pair<u32> nofunc_Read(pci_device*, u16, u8) {
		return zero_pair(cause::NOFUNC);
	}

	// Write
	template<class T> static cause::t call_on_Write(
	    pci_device* x, u16 offset, u8 bytes, u32 value) {
		return static_cast<T*>(x)->on_Write(offset, bytes, value);
	}
	static cause::t nofunc_Write(pci_device*, u16, u8, u32) {
		return cause::NOFUNC;
	}

public:
	pci_device(interfaces* _ifs, const pci_bsf& bsf) :
		ifs(_ifs),
		bus(bsf.pci.bus), slot(bsf.pci.slot), func(bsf.pci.func)
	{}

public:
	// Read interfaces
	cause::pair<u32> read(u16 offset, u8 bytes);

	template <typename _UINT>
	cause::pair<_UINT> read(u16 offset)
	{
		cause::pair<u32> r = read(offset, sizeof (_UINT));
		return cause::pair<_UINT>(r.cause(), r.value());
	}

	void get_bsf(pci_bsf* bsf) {
		*bsf = pci_bsf(bus, slot, func);
	}

	cause::pair<u16> get_vendor_id();
	cause::pair<u16> get_device_id();
	cause::pair<u8>  get_class_prog();
	cause::pair<u16> get_class();
	cause::pair<u8>  get_header_type();
	cause::pair<u32> get_base_address(uint i);

	// Write interfaces
	cause::t write(u16 offset, u8 bytes, u32 value);

	template <typename _UINT>
	cause::t write(u16 offset, _UINT value)
	{
		return write(offset, sizeof (_UINT), value);
	}

public:
	chain_node<pci_device> pci_bus_device_chain_node;

private:
	interfaces* ifs;
	u8 bus;
	u8 slot;
	u8 func;
};

inline cause::pair<u16> pci_device::get_vendor_id() {
	return read<u16>(PCI_VENDOR_ID);
}
inline cause::pair<u16> pci_device::get_device_id() {
	return read<u16>(PCI_DEVICE_ID);
}
inline cause::pair<u8>  pci_device::get_class_prog() {
	return read<u8>(PCI_CLASS_PROG);
}
inline cause::pair<u16> pci_device::get_class() {
	return read<u16>(PCI_CLASS);
}
inline cause::pair<u8>  pci_device::get_header_type() {
	return read<u8>(PCI_HEADER_TYPE);
}
inline cause::pair<u32> pci_device::get_base_address(uint i) {
	if (i > 5)
		return zero_pair(cause::BADARG);
	return read<u32>(PCI_BASE_ADDRESS_0 + sizeof (u32) * i);
}


class pci_bus_device : public bus_device
{
public:
	struct interfaces
	{
		void init();

		using EachDevicesIF = foreach<pci_device*> (*)(pci_bus_device*);
		EachDevicesIF EachDevices;
	};

	// EachDevices
	template<class T> static foreach<pci_device*> call_on_EachDevices(
	    pci_bus_device* x) {
		return static_cast<T*>(x)->on_EachDevices();
	}

	using pci_device_chain =
	      chain<pci_device, &pci_device::pci_bus_device_chain_node>;

public:
	pci_bus_device(interfaces* ifs);
	~pci_bus_device();

public:
	cause::t setup();

	foreach<pci_device*> each_devices();

protected:
	pci_device_chain devices;
	spin_rwlock devices_lock;
private:
	interfaces* _ifs;
};

extern const char device_name_pci[]; // = "pci";


#endif  // CORE_PCI_HH_

