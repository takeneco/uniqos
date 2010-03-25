// @file   include/fileif.hh
// @author Kato Takeshi
// @brief  Device interface class definition.
//
// (C) 2010 Kato Takeshi.

#ifndef _INCLUDE_FILEIF_HH_
#define _INCLUDE_FILEIF_HH_

#include "btypes.hh"


struct IOVector
{
	ucpu  Bytes;
	void* Address;
};


// @brief  File node interface base class.
class FileNodeInterface
{
protected:
	FileNodeInterface() {}

private:
	FileNodeInterface(const FileNodeInterface&);
	void operator = (const FileNodeInterface&);

public:
	virtual int Write(
	    IOVector* Vectors,
	    int       VectorCount,
	    ucpu      Offset);
};


#endif  // Include guard.
