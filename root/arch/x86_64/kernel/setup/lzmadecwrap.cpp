/**
 * @file    arch/x86_86/kernel/setup/lzmadecwrap.cpp
 * @author  Kato Takeshi
 * @brief   LZMA decode wrapper.
 *
 * (C) Kato Takeshi 2009-2010
 */

extern "C" {
#include "LzmaDec.h"
}

#include "mem.hpp"


namespace {

const std::size_t LZMA_HEADER_SIZE = LZMA_PROPS_SIZE + 8;

struct ex_alloc : ISzAlloc {
	memmgr* mm;
};

void* lzma_alloc(void* p, std::size_t size)
{
	ex_alloc* alloc = reinterpret_cast<ex_alloc*>(p);

	return memmgr_alloc(alloc->mm, size);
}

void lzma_free(void* p, void* address)
{
	ex_alloc* alloc = reinterpret_cast<ex_alloc*>(p);

	memmgr_free(alloc->mm, address);
}

}  // End of anonymous namespace.


_u64 lzma_decode_size(const _u8* src)
{
	const _u8* p = src + LZMA_PROPS_SIZE;

	return le64_to_cpu(p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
}

bool lzma_decode(
	memmgr*     mm,
	_u8*        src,
	std::size_t src_len,
	_u8*        dest,
	std::size_t dest_len)
{
	ex_alloc alloc;
	alloc.Alloc = lzma_alloc;
	alloc.Free = lzma_free;
	alloc.mm = mm;

	ELzmaStatus status;

	src_len -= LZMA_HEADER_SIZE;
	return LzmaDecode(
		dest,
		&dest_len,
		src + LZMA_HEADER_SIZE,
		&src_len,
		src,
		LZMA_PROPS_SIZE,
		LZMA_FINISH_END,
		&status,
		&alloc) == SZ_OK;
}
