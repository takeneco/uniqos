// @file   include/fileif.hh
// @author Kato Takeshi
// @brief  Device interface class definition.
//
// (C) 2010 Kato Takeshi
//

#ifndef _INCLUDE_FILEIF_HH_
#define _INCLUDE_FILEIF_HH_

#include "base_types.hh"


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


class file_interface;

struct file_ops
{
	cause::stype (*write)(
	    file_interface* self, const void* data, uptr size, uptr offset);
};

// @brief  file like interface base class.

class file_interface
{
protected:
	file_interface() {}

private:
	file_interface(const file_interface&);
	void operator = (const file_interface&);

public:
	file_ops* ops;

	virtual int write(
	    const io_vector* vectors,
	    int              vector_count,
	    ucpu             offset);
};

typedef file_interface filenode_interface;
typedef file_ops filenode_ops;


#endif  // Include guard.
