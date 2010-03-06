/**
 * @file    arch/x86_64/kernel/setup/lzmadecwrap.hpp
 * @author  Kato Takeshi
 * @brief   Memory ops.
 *
 * (C) Kato Takeshi 2010
 */

#ifndef _ARCH_X86_64_KERNEL_SETUP_LZMADECWRAP_HPP
#define _ARCH_X86_64_KERNEL_SETUP_LZMADECWRAP_HPP

#include "btypes.hpp"


_u64 lzma_decode_size(const _u8* src);
bool lzma_decode(
	memmgr*     mm,
	_u8*        src,
	std::size_t src_len,
	_u8*        dest,
	std::size_t dest_len);


#endif  // Include guard.

