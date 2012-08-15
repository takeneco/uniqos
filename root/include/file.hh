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
	void* base;
	uptr  bytes;
};

class iovec_iterator
{
	const iovec* iov;
	uint         iov_cnt;

	// Current address is iov[iov_index].address[base_offset]
	uptr         iov_index;
	uptr         base_offset;

	void normalize();

public:
	iovec_iterator() :
	    iov(0), iov_cnt(0) {
		reset();
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

class file
{
	DISALLOW_COPY_AND_ASSIGN(file);

public:
	struct operations
	{
		typedef cause::type (*seek_op)(
		    file* x, s64 offset, int whence);
		seek_op seek;

		typedef cause::type (*read_op)(
		    file* x, iovec* iov, int iov_cnt, uptr* bytes);
		read_op read;

		typedef cause::type (*write_op)(
		    file* x, const iovec* iov, int iov_cnt, uptr* bytes);
		write_op write;
	};

	template<class T> static cause::type call_on_seek(
	    file* x, s64 offset, int whence) {
		return static_cast<T*>(x)->on_seek(offset, whence);
	}
	template<class T> static cause::type call_on_read(
	    file* x, iovec* iov, int iov_cnt, uptr* bytes) {
		return static_cast<T*>(x)->on_read(iov, iov_cnt, bytes);
	}
	template<class T> static cause::type call_on_write(
	    file* x, const iovec* iov, int iov_cnt, uptr* bytes) {
		return static_cast<T*>(x)->on_write(iov, iov_cnt, bytes);
	}

	enum seekdir { BEG = 0, CUR, END, };

public:
	cause::type seek(s64 offset, int whence) {
		return ops->seek(this, offset, whence);
	}
	cause::type read(iovec* iov, int iov_cnt, uptr* bytes) {
		return ops->read(this, iov, iov_cnt, bytes);
	}
	cause::type write(const iovec* iov, int iov_cnt, uptr* bytes) {
		return ops->write(this, iov, iov_cnt, bytes);
	}

protected:
	file() {}
	file(const operations* _ops) : ops(_ops) {}

public:
	const operations* ops;

	bool sync;
};


#endif  // include guard

