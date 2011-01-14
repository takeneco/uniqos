// @author  Kato Takeshi
// @brief   LZMA library decode wrapper.
//
// (C) 2010 Kato Takeshi.

#ifndef ARCH_X86_64_KERNEL_SETUP_LZMADECWRAP_HH_
#define ARCH_X86_64_KERNEL_SETUP_LZMADECWRAP_HH_

#include "btypes.hh"
#include "mem.hh"


u64 lzma_decode_size(const _u8* src);
bool lzma_decode(
    _u8*  src,
    uptr  src_len,
    _u8*  dest,
    uptr  dest_len);


#endif  // Include guard.

