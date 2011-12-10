// @file   kernel/file.cc
// @brief  file class implements.
//
// (C) 2010-2011 KATO Takeshi
//

#include "file.hh"


inline void iovec_iterator::normalize()
{
	while (iov_index < iov_cnt) {
		if (base_offset < iov[iov_index].bytes)
			break;
		iov_index++;
		base_offset = 0;
	}
}

// Returns 1 char ptr, and increment.
// @return  Returns next u8 ptr.
// @return  If no buffer, returns null.
u8* iovec_iterator::next_u8()
{
	if (iov_index >= iov_cnt)
		return 0;

	u8* const base = reinterpret_cast<u8*>(iov[iov_index].base);
	u8* const r = &base[base_offset];

	++base_offset;
	normalize();

	return r;
}

uptr iovec_iterator::left_bytes() const
{
	uptr total = 0;

	for (uptr i = iov_index; i < iov_cnt; ++i)
		total += iov[i].bytes;

	return total;
}

