/// @file   core/io_node.hh
/// @brief  io_node class declaration.
//
// (C) 2010-2014 KATO Takeshi
//

#ifndef CORE_INCLUDE_CORE_IO_NODE_HH_
#define CORE_INCLUDE_CORE_IO_NODE_HH_

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
	    iov(0), iov_cnt(0) {
		reset();
	}
	iovec_iterator(uint _iov_cnt, const iovec* _iov) :
	    iov(_iov), iov_cnt(_iov_cnt) {
		reset();
		normalize();
	}
	iovec_iterator(const iovec* iov, ucpu num) :
	    iov(iov), iov_cnt(num) {
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

	struct operations
	{
		void init();

		typedef cause::t (*seek_op)(
		    io_node* x, seek_whence whence,
		    offset rel_off, offset* abs_off);
		seek_op seek;

		typedef cause::t (*read_op)(
		    io_node* x, offset* off, int iov_cnt, iovec* iov);
		read_op read;

		typedef cause::t (*write_op)(
		    io_node* x, offset* off, int iov_cnt, const iovec* iov);
		write_op write;
	};

	// seek
	template<class T> static cause::t call_on_io_node_seek(
	    io_node* x, seek_whence whence, offset rel_off, offset* abs_off) {
		return static_cast<T*>(x)->
		    on_io_node_seek(whence, rel_off, abs_off);
	}
	static cause::t nofunc_io_node_seek(
	    io_node*, seek_whence, offset, offset*) {
		return cause::NOFUNC;
	}

	// read
	template<class T> static cause::t call_on_io_node_read(
	    io_node* x, offset* off, int iov_cnt, iovec* iov) {
		return static_cast<T*>(x)->
		    on_io_node_read(off, iov_cnt, iov);
	}
	static cause::t nofunc_io_node_read(
	    io_node*, offset*, int, iovec*) {
		return cause::NOFUNC;
	}

	// write
	template<class T> static cause::t call_on_io_node_write(
	    io_node* x, offset* off, int iov_cnt, const iovec* iov) {
		return static_cast<T*>(x)->
		    on_io_node_write(off, iov_cnt, iov);
	}
	static cause::t nofunc_io_node_write(
	    io_node*, offset*, int, const iovec*) {
		return cause::NOFUNC;
	}

public:
	cause::t seek(seek_whence whence, offset rel_off, offset* abs_off) {
		return ops->seek(this, whence, rel_off, abs_off);
	}
	cause::t read(offset* off, int iov_cnt, iovec* iov) {
		return ops->read(this, off, iov_cnt, iov);
	}
	cause::t write(offset* off, int iov_cnt, const iovec* iov) {
		return ops->write(this, off, iov_cnt, iov);
	}

protected:
	io_node() {}
	io_node(const operations* _ops) : ops(_ops) {}

	cause::t usual_seek(
	    offset upper_limit, seek_whence whence,
	    offset rel_off, offset* abs_off);

protected:
	const operations* ops;
};


#endif  // include guard

