/// @file   include/file.hh
/// @brief  Device interface class definition.
//
// (C) 2010-2011 KATO Takeshi
//

#ifndef INCLUDE_FILE_HH_
#define INCLUDE_FILE_HH_

#include "basic_types.hh"


struct io_vector
{
	ucpu  bytes;
	void* address;
};

class io_vector_iterator
{
	const io_vector* iovec;
	ucpu             iovec_num;

	// Current address is iov[index].address[offset]
	ucpu             iovec_index;
	ucpu             address_offset;

	void normalize();
public:
	io_vector_iterator() :
	    iovec(0), iovec_num(0) {
		reset();
	}
	explicit io_vector_iterator(const io_vector* iov, ucpu num) :
	    iovec(iov), iovec_num(num) {
		reset();
	}
	void reset() {
		iovec_index = address_offset = 0;
	}
	bool is_end() const {
		return iovec_index >= iovec_num;
	}
	u8* next_u8();
};


class file;

struct file_ops
{
	cause::stype (*write)(
	    file* self, const void* data, uptr size, uptr offset);
};

// @brief  file like interface base class.

class file
{
protected:
	file() {}

private:
	file(const file&);
	void operator = (const file&);

public:
	file_ops* ops;

	int write(
	    const io_vector* vectors,
	    int              vector_count,
	    ucpu             offset);
};

#endif  // Include guard.
