// @file   include/fileif.hh
// @author Kato Takeshi
// @brief  Device interface class definition.
//
// (C) 2010 Kato Takeshi

#ifndef _INCLUDE_FILEIF_HH_
#define _INCLUDE_FILEIF_HH_

#include "btypes.hh"


struct io_vector
{
	ucpu  bytes;
	void* address;
};

class io_vector_iterator
{
	io_vector* iovec;
	ucpu       iovec_num;

	// Current address is iov[index].address[offset]
	ucpu       iovec_index;
	ucpu       address_offset;

	void normalize();
public:
	io_vector_iterator() :
	    iovec(0), iovec_num(0) {
		reset();
	}
	io_vector_iterator(io_vector* iov, ucpu num) :
	    iovec(iov), iovec_num(num) {
		reset();
	}
	void reset() {
		iovec_index = address_offset = 0;
	}
	bool is_end() const {
		return iovec_index < iovec_num &&
		       address_offset < iovec[iovec_index].bytes;
	}
	u8* next_u8();
};

// @brief  File node interface base class.

class filenode_interface
{
protected:
	filenode_interface() {}

private:
	filenode_interface(const filenode_interface&);
	void operator = (const filenode_interface&);

public:
	virtual int write(
	    const io_vector* vectors,
	    int              vector_count,
	    ucpu             offset) =0;
};


#endif  // Include guard.
