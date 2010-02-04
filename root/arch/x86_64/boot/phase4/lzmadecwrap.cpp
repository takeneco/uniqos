/**
 * @file    arch/x86/boot/phase4/lzmadecwrap.cpp
 * @version 0.0.1
 * @date    2009-07-28
 * @author  Kato.T
 * @brief   LZMA decode wrapper.
 */
// (C) Kato.T 2009

extern "C" {
#include "LzmaDec.h"
}

#include "phase4.hpp"


const std::size_t LZMA_HEADER_SIZE = LZMA_PROPS_SIZE + 8;

struct ex_alloc : ISzAlloc {
	memmgr* mm;
};

static void* lzma_alloc(void* p, std::size_t size)
{
	ex_alloc* alloc = reinterpret_cast<ex_alloc*>(p);

	return memmgr_alloc(alloc->mm, size);
}

static void lzma_free(void* p, void* address)
{
	ex_alloc* alloc = reinterpret_cast<ex_alloc*>(p);

	memmgr_free(alloc->mm, p);
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
