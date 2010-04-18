// @file   arch/x86_64/kernel/fileif.cpp
// @author Kato Takeshi
// @brief  DeviceInterface class implements.
//
// (C) 2010 Kato Takeshi.

#include "fileif.hh"


int filenode_interface::write(
    const io_vector* vectors, 
    int              vector_count,
    ucpu             offset)
{
	vectors = vectors;
	vector_count = vector_count;
	offset = offset;

	return cause::INVALID_OPERATION;
}
