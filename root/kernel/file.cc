// @file   kernel/file.cc
// @brief  file class implements.
//
// (C) 2010-2011 KATO Takeshi
//

#include "file.hh"

#include "string.hh"


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
	uptr total = iov[iov_index].bytes - base_offset;

	for (uptr i = iov_index + 1; i < iov_cnt; ++i)
		total += iov[i].bytes;

	return total;
}

/// @return  The number of bytes written.
uptr iovec_iterator::write(uptr bytes, const void* src)
{
	const u8* p = reinterpret_cast<const u8*>(src);

	uptr total = 0;

	while (bytes > 0 && !is_end()) {
		const uptr size =
		    min(iov[iov_index].bytes - base_offset, bytes);
		u8* base = reinterpret_cast<u8*>(iov[iov_index].base);
		mem_copy(size, p, &base[base_offset]);

		p += size;
		bytes -= size;
		base_offset += size;
		total += size;

		normalize();
	}

	return total;
}

/// @return  The number of bytes read.
uptr iovec_iterator::read(uptr bytes, void* dest)
{
	u8* p = reinterpret_cast<u8*>(dest);

	uptr total = 0;

	while (bytes > 0 && !is_end()) {
		const uptr size =
		    min(iov[iov_index].bytes - base_offset, bytes);
		const u8* base = reinterpret_cast<u8*>(iov[iov_index].base);
		mem_copy(size, &base[base_offset], p);

		p += size;
		bytes -= size;
		base_offset += size;
		total += size;

		normalize();
	}

	return total;
}

