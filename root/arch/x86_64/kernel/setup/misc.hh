// @file  misc.hh
//
// (C) 2010-2011 Kato Takeshi
//

#ifndef ARCH_X86_64_KERNEL_SETUP_MISC_HH_
#define ARCH_X86_64_KERNEL_SETUP_MISC_HH_

#include "btypes.hh"
#include "mem.hh"


// log
void setup_log();

// LZMA decode wrapper
u64 lzma_decode_size(const u8* src);
bool lzma_decode(
    u8*  src,
    uptr src_len,
    u8*  dest,
    uptr dest_len);


#endif  // Include guard

