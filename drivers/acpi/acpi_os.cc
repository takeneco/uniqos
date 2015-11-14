/// @file   acpi_os.cc
/// @brief  AcpiOs* interfaces.

//  UNIQOS  --  Unique Operating System
//  (C) 2012-2014 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <cstdarg>
#include <arch/native_io.hh>
#include <config.h>
#include <core/global_vars.hh>
#include <core/intr_ctl.hh>
#include <core/log.hh>
#include <core/mempool.hh>
#include <core/new_ops.hh>
#include <util/string.hh>

#define UNUSE(a) a=a


namespace {

/// checksum
u8 sum8(const void* ptr, u32 length)
{
	const u8* p = reinterpret_cast<const u8*>(ptr);
	u8 sum = 0;
	for (u32 i = 0; i < length; ++i)
		sum += p[i];

	return sum;
}

/// Root System Description Pointer Structure
struct rsdp
{
	char signature[8];
	u8   checksum;
	char oemid[6];
	u8   revision;
	u32  rsdt_adr;
	u32  length;  // maybe sizeof xsdt
	u64  xsdt_adr;
	u8   ext_checksum;
	u8   reserved[3];

	bool test() const;
};

/// test signature and checksum
bool rsdp::test() const
{
	static const char sig[8] = { 'R', 'S', 'D', ' ', 'P', 'T', 'R', ' ' };

	for (int i = 0; i < 8; ++i) {
		if (signature[i] != sig[i])
			return false;
	}

	if (sum8(this, 20 /* ACPI 1.0 spec */) != 0)
		return false;

	return sum8(this, sizeof *this) == 0;
}

/// @return base から base + length の範囲に RSDP が見つかれば、
///         RSDP の物理アドレスを返す。
///         RSDP が見つからなければ 0 を返す。
uptr scan_rsdp(u32 base, u32 length)
{
	const u8* ptr =
	    reinterpret_cast<const u8*>(arch::map_phys_adr(base, length));

	// RSDP must be aligned to 16bytes.

	u32 off;
	for (off = 0; off < length; off += 16) {
		const rsdp* p = reinterpret_cast<const rsdp*>(ptr + off);
		if (p->test())
			break;
	}

	arch::unmap_phys_adr(ptr, length);

	if (off < length)
		return base + off;
	else
		return 0;
}

/// @brief  search RSDP(Root System Description Pointer)
uptr search_rsdp()
{
	// search from EBDA(Extended BIOS Data Area)
	const u16 ebda =
	    *reinterpret_cast<u16*>(arch::map_phys_adr(0x40e, sizeof (u16)));
	uptr rsdp = scan_rsdp(ebda, 1024);

	if (rsdp == 0)
		// search from BIOS
		rsdp = scan_rsdp(0xe0000, 0x1ffff);

	return rsdp;
}

}  // namespace

extern "C" {

#include <acpi.h>

ACPI_STATUS AcpiOsInitialize()
{
	if (CONFIG_DEBUG_ACPI_VERBOSE >= 3) {
		log()(SRCPOS)("()")();
	}

	if (CONFIG_DEBUG_ACPI_VERBOSE < 2) {
		AcpiDbgLevel = 0;
		AcpiDbgLayer = 0;
	} else if (CONFIG_DEBUG_ACPI_VERBOSE == 2) {
		AcpiDbgLayer = ACPI_ALL_COMPONENTS;
		AcpiDbgLevel = ACPI_LV_ALL;
	} else if (CONFIG_DEBUG_ACPI_VERBOSE >= 3) {
		AcpiDbgLayer = ACPI_ALL_COMPONENTS;
		AcpiDbgLevel = ACPI_DEBUG_ALL;
	}

	return AE_OK;
}


ACPI_STATUS
AcpiOsTerminate ()
{
	//TODO
	log()(SRCPOS)("() called")();
	return AE_ERROR;
}


/*
 * ACPI Table interfaces
 */
ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer()
{
	if (CONFIG_DEBUG_VERBOSE >= 2) {
		log()(SRCPOS)("()")();
	}

	ACPI_PHYSICAL_ADDRESS a = search_rsdp();

	if (CONFIG_DEBUG_VERBOSE >= 2) {
		log()(SRCPOS)(": RET 0x").x(a)();
	}

	return a;
}


ACPI_STATUS AcpiOsPredefinedOverride(
    const ACPI_PREDEFINED_NAMES *InitVal,
    ACPI_STRING                 *NewVal)
{
	if (CONFIG_DEBUG_VERBOSE >= 1) {
		log()(SRCPOS)("({\"")
		    .str(InitVal->Name)("\", ")
		    .u(InitVal->Type)(", \"")
		    .str(InitVal->Val)("\"}, ")
		    .p(NewVal)(")")();
	}

	*NewVal = 0;

	return AE_OK;
}


ACPI_STATUS AcpiOsTableOverride(
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_TABLE_HEADER       **NewTable)
{
	// This function cannot output logs, because it will be called before
	// log initialization.

	*NewTable = 0;

	return AE_OK;
}


ACPI_STATUS AcpiOsPhysicalTableOverride(
    ACPI_TABLE_HEADER       *ExistingTable,
    ACPI_PHYSICAL_ADDRESS   *NewAddress,
    UINT32                  *NewTableLength)
{
	// This function cannot output logs, because it will be called before
	// log initialization.

	*NewAddress = 0;
	*NewTableLength = 0;

	return AE_OK;
}


/*
 * Spinlock primitives
 */
ACPI_STATUS
AcpiOsCreateLock (
    ACPI_SPINLOCK           *OutHandle)
{
	if (CONFIG_DEBUG_VERBOSE >= 1) {
		log()(SRCPOS)("(").p(OutHandle)(")")();
	}

	//TODO
	*OutHandle = 0;

	return AE_OK;
}


void
AcpiOsDeleteLock (
    ACPI_SPINLOCK           Handle)
{
	if (CONFIG_DEBUG_VERBOSE >= 1) {
		log()(SRCPOS)("(").p(Handle)(")")();
	}
	//TODO
}


ACPI_CPU_FLAGS
AcpiOsAcquireLock (
    ACPI_SPINLOCK           Handle)
{
	if (CONFIG_DEBUG_VERBOSE >= 1) {
		log()(SRCPOS)("(").p(Handle)(")")();
	}
	//TODO
	return 0;
}


void
AcpiOsReleaseLock (
    ACPI_SPINLOCK           Handle,
    ACPI_CPU_FLAGS          Flags)
{
	if (CONFIG_DEBUG_VERBOSE >= 1) {
		log()(SRCPOS)("(").p(Handle)(", ").x(Flags)(")")();
	}
	//TODO
}


/*
 * Semaphore primitives
 */
ACPI_STATUS
AcpiOsCreateSemaphore (
    UINT32                  MaxUnits,
    UINT32                  InitialUnits,
    ACPI_SEMAPHORE          *OutHandle)
{
	if (CONFIG_DEBUG_VERBOSE >= 2) {
		log()(SRCPOS)("(")
		    .u(MaxUnits)(", ")
		    .u(InitialUnits)(", ")
		    .p(OutHandle)(")")();
	}

	//TODO
	*OutHandle = 0;

	return AE_OK;
}


ACPI_STATUS
AcpiOsDeleteSemaphore (
    ACPI_SEMAPHORE          Handle)
{
	if (CONFIG_DEBUG_VERBOSE >= 2) {
		log()(SRCPOS)("(")(Handle)(") called")();
	}

	//TODO
	return AE_ERROR;
}


ACPI_STATUS
AcpiOsWaitSemaphore (
    ACPI_SEMAPHORE          Handle,
    UINT32                  Units,
    UINT16                  Timeout)
{
	if (CONFIG_DEBUG_VERBOSE >= 1) {
		log()(SRCPOS)("(")
		    .p(Handle)(", ")
		    .u(Units)(", ")
		    .u(Timeout)(")")();
	}

	//TODO

	return AE_OK;
}


ACPI_STATUS
AcpiOsSignalSemaphore (
    ACPI_SEMAPHORE          Handle,
    UINT32                  Units)
{
	if (CONFIG_DEBUG_VERBOSE >= 1) {
		log()(SRCPOS)("(").p(Handle)(", ").u(Units)(")")();
	}

	//TODO

	return AE_OK;
}


/*
 * Mutex primitives. May be configured to use semaphores instead via
 * ACPI_MUTEX_TYPE (see platform/acenv.h)
 */
#if (ACPI_MUTEX_TYPE != ACPI_BINARY_SEMAPHORE)

ACPI_STATUS
AcpiOsCreateMutex (
    ACPI_MUTEX              *OutHandle);

void
AcpiOsDeleteMutex (
    ACPI_MUTEX              Handle);

ACPI_STATUS
AcpiOsAcquireMutex (
    ACPI_MUTEX              Handle,
    UINT16                  Timeout);

void
AcpiOsReleaseMutex (
    ACPI_MUTEX              Handle);
#endif


/*
 * Memory allocation and mapping
 */
void * AcpiOsAllocate(
    ACPI_SIZE               Size)
{
	if (CONFIG_DEBUG_VERBOSE >= 2) {
		log()(SRCPOS)("(").u(Size)(")")();
	}

	void* p = mem_alloc(Size);

	if (CONFIG_DEBUG_VERBOSE >= 2) {
		log()(SRCPOS)(":RET ")(p)();
	}

	return p;
}


void AcpiOsFree(
    void *                  Memory)
{
	if (CONFIG_DEBUG_VERBOSE >= 2) {
		log()(SRCPOS)("(").p(Memory)(")")();
	}

	mem_dealloc(Memory);
}


void * AcpiOsMapMemory(
    ACPI_PHYSICAL_ADDRESS   Where,
    ACPI_SIZE               Length)
{
	// This function cannot output logs, because it will be called before
	// log initialization.

	void* r = arch::map_phys_adr(Where, Length);

	return r;
}


void AcpiOsUnmapMemory(
    void                    *LogicalAddress,
    ACPI_SIZE               Size)
{
	// This function cannot output logs, because it will be called before
	// log initialization.

	uptr r = arch::unmap_phys_adr(LogicalAddress, Size);

	UNUSE(r);
}


ACPI_STATUS
AcpiOsGetPhysicalAddress (
    void                    *LogicalAddress,
    ACPI_PHYSICAL_ADDRESS   *PhysicalAddress);


/*
 * Memory/Object Cache
 */
ACPI_STATUS
AcpiOsCreateCache (
    char                    *CacheName,
    UINT16                  ObjectSize,
    UINT16                  MaxDepth,
    ACPI_CACHE_T            **ReturnCache)
{
	if (CONFIG_DEBUG_VERBOSE >= 2) {
		log()(SRCPOS)("(\"")
		    .str(CacheName)("\", ")
		    .u(ObjectSize)(", ")
		    .u(MaxDepth)(", ")
		    (ReturnCache)(")")();
	}

	auto r = mempool::acquire_shared(ObjectSize);
	if (r.cause() == cause::NOMEM)
		return AE_NO_MEMORY;
	else if (is_fail(r))
		return AE_BAD_PARAMETER;

	mempool* mp = r;
	*ReturnCache = mp;

	if (CONFIG_DEBUG_VERBOSE >= 2) {
		log()(SRCPOS)(" RET ").p(mp)
		             (" objectsize=0x").x(mp->get_obj_size())();
	}

	return AE_OK;
}


ACPI_STATUS
AcpiOsDeleteCache (
    ACPI_CACHE_T            *Cache)
{
	if (CONFIG_DEBUG_VERBOSE >= 1) {
		log()(SRCPOS)("(")(Cache)(")")();
	}

	mempool* mp = static_cast<mempool*>(Cache);

	mempool::release_shared(mp);

	return AE_OK;
}


ACPI_STATUS
AcpiOsPurgeCache (
    ACPI_CACHE_T            *Cache)
{
	if (CONFIG_DEBUG_VERBOSE >= 2) {
		log()(SRCPOS)("(")(Cache)(") called")();
	}

	//TODO
	return AE_ERROR;
}


void *
AcpiOsAcquireObject (
    ACPI_CACHE_T            *Cache)
{
	//TODO
	if (CONFIG_DEBUG_VERBOSE >= 2) {
		log()(SRCPOS)("(")(Cache)(")")();
	}

	mempool* mp = static_cast<mempool*>(Cache);
	void* r = mp->alloc();

	mem_fill(mp->get_obj_size(), 0, r);

	if (CONFIG_DEBUG_VERBOSE >= 2) {
		log()(SRCPOS)(":RET ")(r)();
	}

	return r;
}


ACPI_STATUS
AcpiOsReleaseObject (
    ACPI_CACHE_T            *Cache,
    void                    *Object)
{
	//TODO
	if (CONFIG_DEBUG_VERBOSE >= 2) {
		log()(SRCPOS)("(")(Cache)(", ")(Object)(")")();
	}

	mempool* mp = static_cast<mempool*>(Cache);
	mp->dealloc(Object);

	return AE_OK;
}

/*
 * Interrupt handlers
 */

class acpi_intr_handler : public intr_handler
{
public:
	acpi_intr_handler(ACPI_OSD_HANDLER _ServiceRoutine, void* _Context) :
		intr_handler(on_intr),
		ServiceRoutine(_ServiceRoutine),
		Context(_Context)
	{}

private:
	static void on_intr(intr_handler* h);

private:
	ACPI_OSD_HANDLER ServiceRoutine;
	void* Context;
};

void acpi_intr_handler::on_intr(intr_handler* intr_h)
{
	acpi_intr_handler* h = static_cast<acpi_intr_handler*>(intr_h);

	const u32 r = h->ServiceRoutine(h->Context);

	if (r == ACPI_INTERRUPT_HANDLED) {
	} else if (r == ACPI_INTERRUPT_NOT_HANDLED) {
		// @todo : confirm
	}
}


ACPI_STATUS
AcpiOsInstallInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine,
    void                    *Context)
{
	if (CONFIG_DEBUG_VERBOSE >= 0) {
		log()(SRCPOS)("(")
		    .u(InterruptNumber)(", ")
		    (reinterpret_cast<void*>(ServiceRoutine))(", ")
		    (Context)(")")();
	}

	acpi_intr_handler* intr_h =
	    new (mem_alloc(sizeof (acpi_intr_handler)))
	    acpi_intr_handler(ServiceRoutine, Context);

	cause::type r = global_vars::core.intr_ctl_obj->install_handler(
	    InterruptNumber, intr_h);
	if (is_fail(r))
		return AE_ERROR;

	//TODO
	return AE_OK;
}


ACPI_STATUS
AcpiOsRemoveInterruptHandler (
    UINT32                  InterruptNumber,
    ACPI_OSD_HANDLER        ServiceRoutine)
{
	//TODO
	UNUSE(InterruptNumber);
	UNUSE(ServiceRoutine);
	log()(SRCPOS)("() called")();
	return AE_ERROR;
}


/*
 * Threads and Scheduling
 */
ACPI_THREAD_ID
AcpiOsGetThreadId ()
{
	if (CONFIG_DEBUG_VERBOSE >= 1) {
		log()(SRCPOS)("() called")();
	}

	//return 0;
	return 1;
}


ACPI_STATUS
AcpiOsExecute (
    ACPI_EXECUTE_TYPE       Type,
    ACPI_OSD_EXEC_CALLBACK  Function,
    void                    *Context)
{
	//TODO
	UNUSE(Type);
	UNUSE(Function);
	UNUSE(Context);
	log()(SRCPOS)("() called")();
	return AE_ERROR;
}


void
AcpiOsWaitEventsComplete ()
{
	//TODO
	log()(SRCPOS)("() called")();
}

void
AcpiOsSleep (
    UINT64  Milliseconds)
{
	//TODO
	log()(SRCPOS)("(").u(Milliseconds)(") called")();
}


void
AcpiOsStall (
    UINT32                  Microseconds)
{
	//TODO
	log()(SRCPOS)("(").u(Microseconds)(") called")();
}


/*
 * Platform and hardware-independent I/O interfaces
 */
ACPI_STATUS
AcpiOsReadPort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  *Value,
    UINT32                  Width)
{
	if (CONFIG_DEBUG_VERBOSE >= 1) {
		log()(SRCPOS)("(")
		    .x(Address)(", ")
		    (Value)(", ")
		    .u(Width)(")")();
	}

	switch (Width) {
	case 8:
		*Value = arch::ioport_in8(Address);
		break;
	case 16:
		*Value = arch::ioport_in16(Address);
		break;
	case 32:
		*Value = arch::ioport_in32(Address);
		break;
	default:
		log()("!!!")(SRCPOS)(" Unexpected. Width=").u(Width)();
		return AE_ERROR;
	}

	if (CONFIG_DEBUG_VERBOSE >= 1) {
		log()(SRCPOS)(" RET: *Value = 0x").x(*Value)();
	}

	return AE_OK;
}


ACPI_STATUS
AcpiOsWritePort (
    ACPI_IO_ADDRESS         Address,
    UINT32                  Value,
    UINT32                  Width)
{
	if (CONFIG_DEBUG_VERBOSE >= 1) {
		log()(SRCPOS)("(")
		    .x(Address)(", ")
		    .u(Value)(", ")
		    .u(Width)(")")();
	}

	switch (Width) {
	case 8:
		arch::ioport_out8(Value, Address);
		break;
	case 16:
		arch::ioport_out16(Value, Address);
		break;
	case 32:
		arch::ioport_out32(Value, Address);
		break;
	default:
		log()("!!!")(SRCPOS)(" Unexpected. Width=").u(Width)();
		return AE_ERROR;
	}

	return AE_OK;
}


/*
 * Platform and hardware-independent physical memory interfaces
 */
ACPI_STATUS
AcpiOsReadMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  *Value,
    UINT32                  Width)
{
	//TODO
	UNUSE(Address);
	UNUSE(Value);
	UNUSE(Width);
	log()(SRCPOS)("() called")();
	return AE_ERROR;
}


ACPI_STATUS
AcpiOsWriteMemory (
    ACPI_PHYSICAL_ADDRESS   Address,
    UINT64                  Value,
    UINT32                  Width)
{
	//TODO
	UNUSE(Address);
	UNUSE(Value);
	UNUSE(Width);
	log()(SRCPOS)("() called")();
	return AE_ERROR;
}


/*
 * Platform and hardware-independent PCI configuration space access
 * Note: Can't use "Register" as a parameter, changed to "Reg" --
 * certain compilers complain.
 */
ACPI_STATUS
AcpiOsReadPciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  *Value,
    UINT32                  Width)
{
	//TODO:delete
	log()(SRCPOS)("() called")();
	log()("Seg=").x(PciId->Segment)
	(" Bus=").x(PciId->Bus)
	(" Dev=").x(PciId->Device)
	(" Fuc=").x(PciId->Function)
	(" Reg=").x(Reg)
	(" Width=").u(Width);

	if (CONFIG_DEBUG_VALIDATE >= 1) {
		if (PciId->Bus >= 256 ||
		    PciId->Device > 32 ||
		    PciId->Function >= 8)
		{
			return AE_BAD_PARAMETER;
		}
	}

	u32 config_address = U32(0x80000000) |
	    (static_cast<u32>(PciId->Bus) << 16) |
	    (static_cast<u32>(PciId->Device) << 11) |
	    (static_cast<u32>(PciId->Function) << 8) |
	    (static_cast<u32>(Reg) & ~3);


	if (Width <= 32) {
		arch::ioport_out32(config_address, 0xcf8);
		u32 val = arch::ioport_in32(0xcfc);
		log()(" Val=").x(val);
		val >>= (Reg & 3) * 8;
		val &= (1 << Width) - 1;
		*Value = val;
	} else if (Width == 64) {
		arch::ioport_out32(config_address, 0xcf8);
		u64 val = arch::ioport_in32(0xcfc);
		log()(" Val=").x(val);
		arch::ioport_out32(config_address + 4, 0xcf8);
		val |= static_cast<u64>(arch::ioport_in32(0xcfc)) << 32;
		*Value = val;
	} else {
		return AE_BAD_PARAMETER;
	}

	log()("(").x(*Value)(")")();

	return AE_OK;
}


ACPI_STATUS
AcpiOsWritePciConfiguration (
    ACPI_PCI_ID             *PciId,
    UINT32                  Reg,
    UINT64                  Value,
    UINT32                  Width)
{
	//TODO
	UNUSE(PciId);
	UNUSE(Reg);
	UNUSE(Value);
	UNUSE(Width);
	log()(SRCPOS)("() called")();
	log()("Seg=").x(PciId->Segment)
	(" Bus=").x(PciId->Bus)
	(" Dev=").x(PciId->Device)
	(" Fuc=").x(PciId->Function)();
	log()("Reg=").x(Reg)(" Val=").x(Value)(" Width=").u(Width)();
	return AE_ERROR;
}


/*
 * Miscellaneous
 */
BOOLEAN
AcpiOsReadable (
    void                    *Pointer,
    ACPI_SIZE               Length);

BOOLEAN
AcpiOsWritable (
    void                    *Pointer,
    ACPI_SIZE               Length);

UINT64
AcpiOsGetTimer ()
{
	//TODO
	log()(SRCPOS)("() called")();
	return 0;
}


ACPI_STATUS
AcpiOsSignal (
    UINT32                  Function,
    void                    *Info)
{
	//TODO
	UNUSE(Function);
	UNUSE(Info);
	log()(SRCPOS)("() called")();
	return AE_ERROR;
}


void ACPI_INTERNAL_VAR_XFACE
AcpiOsPrintf (
    const char              *Format,
    ...)
{
	if (global_vars::core.log_target_objs) {
		va_list va;
		va_start(va, Format);

		log().format(Format, va);

		va_end(va);
	}
}


void
AcpiOsVprintf (
    const char              *Format,
    va_list                 Args)
{
	if (global_vars::core.log_target_objs)
		log().format(Format, Args);
}


void
AcpiOsRedirectOutput (
    void                    *Destination);


/*
 * Debug input
 */
ACPI_STATUS
AcpiOsGetLine (
    char                    *Buffer,
    UINT32                  BufferLength,
    UINT32                  *BytesRead);


/*
 * Directory manipulation
 */
void *
AcpiOsOpenDirectory (
    char                    *Pathname,
    char                    *WildcardSpec,
    char                    RequestedFileType);

/* RequesteFileType values */

//#define REQUEST_FILE_ONLY                   0
//#define REQUEST_DIR_ONLY                    1


char *
AcpiOsGetNextFilename (
    void                    *DirHandle);

void
AcpiOsCloseDirectory (
    void                    *DirHandle);




////////////////
void
AcpiOsDerivePciId(
    ACPI_HANDLE             Rhandle,
    ACPI_HANDLE             Chandle,
    ACPI_PCI_ID             **PciId)
{
	//TODO
	log()(SRCPOS)("(")(Rhandle)(", ")(Chandle)(", ")(PciId)(") called")();
}
ACPI_STATUS
AcpiOsValidateInterface (
    char                    *Interface)
{
	//TODO
	log()(SRCPOS)("(")(Interface)(") called")();
	return AE_ERROR;
}
////////////////

}  // extern "C"

