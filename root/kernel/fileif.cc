// @file   arch/x86_64/kernel/fileif.cpp
// @author Kato Takeshi
// @brief  DeviceInterface class implements.
//
// (C) Kato Takeshi 2010

#include "fileif.hh"


int FileNodeInterface::Write(
    IOVector* Vectors, 
    int       VectorCount,
    ucpu      Offset)
{
	Vectors = Vectors;
	VectorCount = VectorCount;
	Offset = Offset;

	return cause::INVALID_OPERATION;
}
