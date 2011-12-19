/// @file   include/file.hh
/// @brief  file class declaration.
//
// (C) 2010-2011 KATO Takeshi
//

#ifndef INCLUDE_FILE_HH_
#define INCLUDE_FILE_HH_

#include "basic_types.hh"


struct iovec
{
	void* base;
	uptr  bytes;
};

class iovec_iterator
{
	const iovec* iov;
	uint         iov_cnt;

	// Current address is iov[iov_index].address[base_offset]
	uptr         iov_index;
	uptr         base_offset;

	void normalize();

public:
	iovec_iterator() :
	    iov(0), iov_cnt(0) {
		reset();
	}
	iovec_iterator(const iovec* iov, ucpu num) :
	    iov(iov), iov_cnt(num) {
		reset();
	}
	void reset() {
		iov_index = base_offset = 0;
	}
	bool is_end() const {
		return iov_index >= iov_cnt;
	}
	u8* next_u8();
	uptr left_bytes() const;
};


// @brief  file like interface base class.

class file
{
public:
	struct operations
	{
		typedef cause::stype (*seek_fn)(
		    file* x, s64 offset, int whence);
		seek_fn seek;

		typedef cause::stype (*read_fn)(
		    file* x, iovec* iov, int iov_cnt);
		read_fn read;

		typedef cause::stype (*write_fn)(
		    file* x, const iovec* iov, int iov_cnt, uptr* bytes);
		write_fn write;
	};

	enum seekdir { BEG = 0, CUR, END, };

public:
	cause::stype write(const iovec* iov, int iov_cnt, uptr* bytes) {
		return ops->write(this, iov, iov_cnt, bytes);
	}

protected:
	file() {}

private:
	file(const file&);
	void operator = (const file&);

public:
	const operations* ops;
};


#endif  // include guard

