/// @author  KATO Takeshi
/// @brief   LZMA decode wrapper.
//
// (C) 2009-2010 KATO Takeshi

#include "lzmadecwrap.hh"

extern "C" {
#include "LzmaDec.h"
}

#include "mem.hh"


namespace {

	const std::size_t LZMA_HEADER_SIZE = LZMA_PROPS_SIZE + 8;

	void* lzma_alloc(void*, std::size_t size)
	{
		void* p = memmgr_alloc(size);
		return p;
	}

	void lzma_free(void*, void* addr)
	{
		memmgr_free(addr);
	}

}  // namespace


u64 lzma_decode_size(const _u8* src)
{
	const u8* p = src + LZMA_PROPS_SIZE;

	return le64_to_cpu(p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
}

bool lzma_decode(
	u8*         src,
	std::size_t src_len,
	u8*         dest,
	std::size_t dest_len)
{
	ISzAlloc alloc;
	alloc.Alloc = lzma_alloc;
	alloc.Free = lzma_free;

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
