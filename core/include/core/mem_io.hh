/// @file   include/mem_io.hh
/// @brief  mem_io class declaration.
//
// (C) 2012-2014 KATO Takeshi
//

#ifndef INCLUDE_MEM_IO_HH_
#define INCLUDE_MEM_IO_HH_

#include <core/io_node.hh>


//TODO:dynamic
extern io_node::interfaces mem_io_node_ifs;

/// @brief  On memory io_node.
class mem_io : public io_node
{
	DISALLOW_COPY_AND_ASSIGN(mem_io);

	friend cause::t mem_io_setup();

public:
	mem_io(void* _contents, uptr bytes);

	template<offset BYTES> mem_io(char (&_contents)[BYTES]) :
		io_node(&mem_io_node_ifs),
		contents(reinterpret_cast<u8*>(_contents)),
		capacity_bytes(BYTES)
	{}

	cause::t on_io_node_seek(
	    seek_whence whence, offset rel_off, offset* abs_off);
	cause::pair<uptr> on_Read(offset off, void* data, uptr bytes);
	cause::t on_io_node_read(
	    offset* off, int iov_cnt, iovec* iov);
	cause::pair<uptr> on_Write(offset off, const void* data, uptr bytes);
	cause::t on_io_node_write(
	    offset* off, int iov_cnt, const iovec* iov);

private:
	static cause::t setup();

private:
	u8* contents;
	offset capacity_bytes;
};


/// @brief  Ringed on memory io_node.
class ringed_mem_io : public io_node
{
	DISALLOW_COPY_AND_ASSIGN(ringed_mem_io);

	friend cause::t mem_io_setup();

public:
	ringed_mem_io(void* _contents, uptr bytes);

	cause::t on_io_node_seek(
	    seek_whence whence, offset rel_off, offset* abs_off);
	cause::pair<uptr> on_Read(offset off, void* data, uptr bytes);
	cause::t on_io_node_read(
	    offset* off, int iov_cnt, iovec* iov);
	cause::pair<uptr> on_Write(offset off, const void* data, uptr bytes);
	cause::t on_io_node_write(
	    offset* off, int iov_cnt, const iovec* iov);

	uoffset offset_normalize(offset off);

private:
	static cause::t setup();

private:
	u8* contents;
	offset capacity_bytes;
};


#endif  // include guard

