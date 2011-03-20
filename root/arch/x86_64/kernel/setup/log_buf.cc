/// @file  log_buf.cc
/// @brief memory stored log. reading after setup.
//
// (C) 2011 KATO Takeshi
//

#include "misc.hh"
#include "setup.h"


namespace {

inline u8* get_buf_start() {
	return reinterpret_cast<u8*>(
	    (SETUP_LOGBUF_SEG << 4) + SETUP_LOGBUF_ADR);
}

inline u32* get_current() {
	return reinterpret_cast<u32*>(
	    (SETUP_DATA_SEG << 4) + SETUP_LOGBUF_CUR);
}

} // namespace

on_memory_log::on_memory_log()
    : kernel_log(write)
{
	*get_current() = 0;
}

void on_memory_log::write(kernel_log* , const void* data, u32 bytes)
{
	u8* buf_start = get_buf_start();
	u32* current = get_current();

	u32 cur = *current;
	for (u32 i = 0; i < bytes; ++i) {
		if (cur >= SETUP_LOGBUF_SIZE)
			break;
		buf_start[cur] = reinterpret_cast<const u8*>(data)[i];
		++cur;
	}

	*current += bytes;
}

