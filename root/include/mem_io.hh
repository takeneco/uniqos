/// @file   include/mem_io.hh
/// @brief  mem_io class declaration.
//
// (C) 2012-2013 KATO Takeshi
//

#ifndef INCLUDE_MEM_IO_HH_
#define INCLUDE_MEM_IO_HH_

#include <io_node.hh>


extern io_node::operations mem_io_node_ops;

/// @brief  On memory io_node.
class mem_io : public io_node
{
	DISALLOW_COPY_AND_ASSIGN(mem_io);

	friend cause::type mem_io_setup();

public:
	mem_io(uptr bytes, void* _contents);

	template<offset BYTES> mem_io(char (&_contents)[BYTES]) :
		io_node(&mem_io_node_ops),
		contents(reinterpret_cast<u8*>(_contents)),
		capacity_bytes(BYTES)
	{}

	cause::type on_io_node_seek(
	    seek_whence whence, offset rel_off, offset* abs_off);
	cause::type on_io_node_read(
	    offset* off, int iov_cnt, iovec* iov);
	cause::type on_io_node_write(
	    offset* off, int iov_cnt, const iovec* iov);

private:
	static cause::type setup();

private:
	u8* contents;
	offset capacity_bytes;
};


/// @brief  Ringed on memory io_node.
class ringed_mem_io : public io_node
{
	DISALLOW_COPY_AND_ASSIGN(ringed_mem_io);

	friend cause::type mem_io_setup();

public:
	ringed_mem_io(uptr bytes, void* _contents);

	cause::type on_io_node_seek(
	    seek_whence whence, offset rel_off, offset* abs_off);
	cause::type on_io_node_read(
	    offset* off, int iov_cnt, iovec* iov);
	cause::type on_io_node_write(
	    offset* off, int iov_cnt, const iovec* iov);

	offset offset_normalize(offset off);

private:
	static cause::type setup();

private:
	u8* contents;
	offset capacity_bytes;
};


#endif  // include guard

