/// @file  log_buf.cc
/// @brief memory stored log. reading after setup.
//
// (C) 2011 KATO Takeshi
//

#include "log.hh"
#include "misc.hh"
#include "setup.h"



namespace {

class log_buf : public kernel_log
{
public:
	log_buf() : kernel_log(write) {}

private:
	static void write(kernel_log* , const u8* data, u32 bytes);
};

void log_buf::write(kernel_log* , const u8* data, u32 bytes)
{
	u8* buf_start = reinterpret_cast<u8*>(
	    SETUP_LOGBUF_SEG << 4 + SETUP_LOGBUF_ADR);
	u32* current = reinterpret_cast<u32*>(
	    SETUP_DATA_SEG << 4 + SETUP_LOGBUF_CUR);

	u32 cur = *current;
	for (u32 i = 0; i < bytes; ++i) {
		if (cur >= SETUP_LOGBUF_SIZE)
			cur = 0;
		buf_start[cur] = data[i];
		++cur;
	}

	*current += bytes;
}

}  // namespace


void setup_log()
{
}
