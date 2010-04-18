// @file   include/fileif.hh
// @author Kato Takeshi
// @brief  Device interface class definition.
//
// (C) 2010 Kato Takeshi.

#ifndef _INCLUDE_FILEIF_HH_
#define _INCLUDE_FILEIF_HH_

#include "btypes.hh"


struct io_vector
{
	ucpu  bytes;
	void* address;
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
	    ucpu             offset);
};


#endif  // Include guard.
