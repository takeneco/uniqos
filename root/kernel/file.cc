// @file   arch/x86_64/kernel/file.cpp
// @author Kato Takeshi
// @brief  DeviceInterface class implements.
//
// (C) 2010 Kato Takeshi

#include "file.hh"


inline void io_vector_iterator::normalize()
{
	while (iovec_index < iovec_num) {
		if (address_offset < iovec[iovec_index].bytes)
			break;
		iovec_index++;
		address_offset = 0;
	}
}

// Returns 1 char ptr, and increment.
// @return  Returns next u8 ptr.
// @return  If no buffer, returns null.
u8* io_vector_iterator::next_u8()
{
	if (iovec_index >= iovec_num) {
		return 0;
	}
	//if (address_offset >= iovec[iovec_index].bytes) {
	//	return 0;
	//}

	u8* const addr = reinterpret_cast<u8*>(iovec[iovec_index].address);
	u8* const r = &addr[address_offset];

	address_offset++;
	normalize();

	return r;
}

int file::write(const io_vector*, int, ucpu)
{
	return cause::INVALID_OPERATION;
}
