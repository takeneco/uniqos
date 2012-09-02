/// @file   include/file.hh
/// @brief  file class declaration.
//
// (C) 2010-2012 KATO Takeshi
//

#ifndef INCLUDE_FILE_HH_
#define INCLUDE_FILE_HH_

#include <basic.hh>


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
// TODO:io_node
class file
{
	DISALLOW_COPY_AND_ASSIGN(file);

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
		typedef cause::type (*seek_op)(
		    file* x, seek_whence whence,
		    offset rel_off, offset* abs_off);
		seek_op seek;

		typedef cause::type (*read_op)(
		    file* x, offset* off, int iov_cnt, iovec* iov);
		read_op read;

		typedef cause::type (*write_op)(
		    file* x, offset* off, int iov_cnt, const iovec* iov);
		write_op write;
	};

	template<class T> static cause::type call_on_file_seek(
	    file* x, seek_whence whence, offset rel_off, offset* abs_off) {
		return static_cast<T*>(x)->
		    on_file_seek(whence, rel_off, abs_off);
	}
	template<class T> static cause::type call_on_file_read(
	    file* x, offset* off, int iov_cnt, iovec* iov) {
		return static_cast<T*>(x)->
		    on_file_read(off, iov_cnt, iov);
	}
	template<class T> static cause::type call_on_file_write(
	    file* x, offset* off, int iov_cnt, const iovec* iov) {
		return static_cast<T*>(x)->
		    on_file_write(off, iov_cnt, iov);
	}

public:
	cause::type seek(seek_whence whence, offset rel_off, offset* abs_off) {
		return ops->seek(this, whence, rel_off, abs_off);
	}
	cause::type read(offset* off, int iov_cnt, iovec* iov) {
		return ops->read(this, off, iov_cnt, iov);
	}
	cause::type write(offset* off, int iov_cnt, const iovec* iov) {
		return ops->write(this, off, iov_cnt, iov);
	}

protected:
	file() {}
	file(const operations* _ops) : ops(_ops) {}

	cause::type usual_seek(
	    offset upper_limit, seek_whence whence,
	    offset rel_off, offset* abs_off);

public:
	const operations* ops;

	bool sync;
};


#endif  // include guard

