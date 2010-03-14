// @file    arch/x86_64/kernel/setup/lzmadecwrap.hpp
// @author  Kato Takeshi
// @brief   LZMA library decode wrapper.
//
// (C) 2010 Kato Takeshi.

#ifndef _ARCH_X86_64_KERNEL_SETUP_LZMADECWRAP_HH_
#define _ARCH_X86_64_KERNEL_SETUP_LZMADECWRAP_HH_

#include "btypes.hh"
#include "mem.hh"


_u64 lzma_decode_size(const _u8* src);
bool lzma_decode(
	memmgr*     mm,
	_u8*        src,
	std::size_t src_len,
	_u8*        dest,
	std::size_t dest_len);


#endif  // Include guard.

