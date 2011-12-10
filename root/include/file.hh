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


class file;

struct file_ops
{
	typedef cause::stype (*write_fn)(
	    file* x, const iovec* iov, int iov_cnt);
	write_fn write;
};

// @brief  file like interface base class.

class file
{
public:
	cause::stype write(const iovec* iov, int iov_cnt) {
		return ops->write(this, iov, iov_cnt);
	}

protected:
	file() {}

private:
	file(const file&);
	void operator = (const file&);

public:
	file_ops* ops;
};


#endif  // include guard

