/// @file   core/io_node.hh
/// @brief  io_node class declaration.
//
// (C) 2010-2014 KATO Takeshi
//

#ifndef CORE_IO_NODE_HH_
#define CORE_IO_NODE_HH_

#include <core/basic.hh>


struct iovec
{
	uptr  bytes;
	void* base;
};

class iovec_iterator
{
	const iovec* iov;
	uint         iov_cnt;

	// Current address is iov[iov_index].address[base_offset]
	uint         iov_index;
	uptr         base_offset;

	void normalize();

public:
	iovec_iterator() :
		iov(0), iov_cnt(0)
	{
		reset();
	}
	iovec_iterator(uint _iov_cnt, const iovec* _iov) :
		iov(_iov), iov_cnt(_iov_cnt)
	{
		reset();
		normalize();
	}
	iovec_iterator(const iovec* iov, ucpu num) :
		iov(iov), iov_cnt(num)
	{
		reset();
		normalize();
	}
	void reset() {
		iov_index = base_offset = 0;
	}
	bool is_end() const {
		return iov_index >= iov_cnt;
	}
	u8* next_u8();
	uptr left_bytes() const;

	uptr write(uptr bytes, const void* src);
	uptr read(uptr bytes, void* dest);
};


struct dir_entry
{
	u32  record_bytes;
	u32  name_bytes;
	char name[];
};


// @brief  file like interface base class.
class io_node
{
	DISALLOW_COPY_AND_ASSIGN(io_node);

public:
	enum seek_whence { BEG = 0, ADD, END, };

	typedef s64 offset;
	typedef u64 uoffset;
	enum {
		OFFSET_MAX = U64(0x7fffffffffffffff),
		OFFSET_MIN = S64(-0x8000000000000000),
	};

	struct interfaces
	{
		void init();

		typedef cause::t (*CloseIF)(
		    io_node* x);
		CloseIF Close;

		typedef cause::t (*SeekIF)(
		    io_node* x, seek_whence whence,
		    offset rel_off, offset* abs_off);
		SeekIF seek;

		typedef cause::pair<uptr> (*ReadIF)(
		    io_node* x, offset off, void* data, uptr bytes);
		ReadIF Read;

		typedef cause::t (*ReadVIF)(
		    io_node* x, offset* off, int iov_cnt, iovec* iov);
		ReadVIF read;

		typedef cause::pair<uptr> (*WriteIF)(
		    io_node* x, offset off, const void* data, uptr bytes);
		WriteIF Write;

		typedef cause::t (*WriteVIF)(
		    io_node* x, offset* off, int iov_cnt, const iovec* iov);
		WriteVIF write;

		typedef cause::pair<dir_entry*> (*GetDirEntryIF)(
		    io_node* x, uptr buf_bytes, dir_entry* buf);
		GetDirEntryIF GetDirEntry;
	};

	// Close
	template<class T> static cause::t call_on_Close(io_node* x) {
		return static_cast<T*>(x)->on_Close(static_cast<T*>(x));
	}
	static cause::t nofunc_Close(io_node*) {
		return cause::NOFUNC;
	}

	// seek
	template<class T> static cause::t call_on_io_node_seek(
	    io_node* x, seek_whence whence, offset rel_off, offset* abs_off) {
		return static_cast<T*>(x)->
		    on_io_node_seek(whence, rel_off, abs_off);
	}
	static cause::t nofunc_Seek(
	    io_node*, seek_whence, offset, offset*) {
		return cause::NOFUNC;
	}

	// Read
	template<class T> static cause::pair<uptr> call_on_Read(
	    io_node* x, offset off, void* data, uptr bytes) {
		return static_cast<T*>(x)->
		    on_Read(off, data, bytes);
	}
	static cause::pair<uptr> nofunc_Read(
	    io_node*, offset, void*, uptr) {
		return zero_pair(cause::NOFUNC);
	}

	// readv
	template<class T> static cause::t call_on_io_node_read(
	    io_node* x, offset* off, int iov_cnt, iovec* iov) {
		return static_cast<T*>(x)->
		    on_io_node_read(off, iov_cnt, iov);
	}
	static cause::t nofunc_ReadV(
	    io_node*, offset*, int, iovec*) {
		return cause::NOFUNC;
	}

	// Write
	template<class T> static cause::pair<uptr> call_on_Write(
	    io_node* x, offset off, const void* data, uptr bytes) {
		return static_cast<T*>(x)->
		    on_Write(off, data, bytes);
	}
	static cause::pair<uptr> nofunc_Write(
	    io_node*, offset, const void*, uptr) {
		return zero_pair(cause::NOFUNC);
	}

	// writev
	template<class T> static cause::t call_on_io_node_write(
	    io_node* x, offset* off, int iov_cnt, const iovec* iov) {
		return static_cast<T*>(x)->
		    on_io_node_write(off, iov_cnt, iov);
	}
	static cause::t nofunc_WriteV(
	    io_node*, offset*, int, const iovec*) {
		return cause::NOFUNC;
	}

	// GetDirEntry
	template<class T>
	static cause::pair<dir_entry*> call_on_GetDirEntry(
	    io_node* x, uptr buf_bytes, dir_entry* buf) {
		return static_cast<T*>(x)->
		    on_GetDirEntry(buf_bytes, buf);
	}
	static cause::pair<dir_entry*> nofunc_GetDirEntry(
	    io_node*, uptr, dir_entry*) {
		return null_pair(cause::NOFUNC);
	}

public:
	static cause::t close(io_node* x) {
		return x->ifs->Close(x);
	}
	cause::t seek(seek_whence whence, offset rel_off, offset* abs_off) {
		return ifs->seek(this, whence, rel_off, abs_off);
	}
	cause::pair<uptr> read(offset off, void* data, uptr bytes) {
		return ifs->Read(this, off, data, bytes);
	}
	cause::t readv(offset* off, int iov_cnt, iovec* iov) {
		return ifs->read(this, off, iov_cnt, iov);
	}
	cause::pair<uptr> write(offset off, const void* data, uptr bytes) {
		return ifs->Write(this, off, data, bytes);
	}
	cause::t writev(offset* off, int iov_cnt, const iovec* iov) {
		return ifs->write(this, off, iov_cnt, iov);
	}
	cause::pair<dir_entry*> get_dir_entry(
	    uptr buf_bytes, dir_entry* buf) {
		return ifs->GetDirEntry(this, buf_bytes, buf);
	}

protected:
	io_node() {}
	io_node(const interfaces* _ifs) : ifs(_ifs) {}

	cause::t usual_seek(
	    offset upper_limit, seek_whence whence,
	    offset rel_off, offset* abs_off);

protected:
	const interfaces* ifs;
};


#endif  // include guard

