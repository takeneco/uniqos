/// @file   new_ops.hh
//
// (C) 2010,2013-2014 KATO Takeshi
//

#ifndef CORE_NEW_OPS_HH_
#define CORE_NEW_OPS_HH_

#include <core/basic.hh>


class mem_allocator
{
public:
	struct operations
	{
		void init() {
			allocate = nofunc_mem_allocator_allocate;
			deallocate = nofunc_mem_allocator_deallocate;
		}

		typedef cause::pair<void*> (*allocate_op)(
		    mem_allocator* x, uptr bytes);
		allocate_op allocate;

		typedef cause::t (*deallocate_op)(
		    mem_allocator* x, void* p);
		deallocate_op deallocate;
	};

	mem_allocator() {}
	mem_allocator(const operations* _ops) : ops(_ops) {}

protected:
	template<class T>
	static cause::pair<void*> call_on_mem_allocator_allocate(
	    mem_allocator* x, uptr bytes) {
		return static_cast<T*>(x)->on_mem_allocator_allocate(bytes);
	}
	static cause::pair<void*> nofunc_mem_allocator_allocate(
	    mem_allocator*, uptr) {
		return null_pair(cause::NOFUNC);
	}

	template<class T>
	static cause::t call_on_mem_allocator_deallocate(
	    mem_allocator* x, void* p) {
		return static_cast<T*>(x)->on_mem_allocator_deallocate(p);
	}
	static cause::t nofunc_mem_allocator_deallocate(
	    mem_allocator*, void*) {
		return cause::NOFUNC;
	}

public:
	cause::pair<void*> allocate(uptr bytes) {
		return ops->allocate(this, bytes);
	}

	cause::t deallocate(void* p) {
		return ops->deallocate(this, p);
	}

protected:
	const operations* ops;
};

inline void* operator new  (uptr, void* ptr) { return ptr; }
inline void* operator new[](uptr, void* ptr) { return ptr; }

inline void operator delete  (void*, void*) {}
inline void operator delete[](void*, void*) {}

inline void* operator new (uptr size, mem_allocator& alloc) {
	auto r = alloc.allocate(size);
	if (is_ok(r))
		return r.get_data();
	else
		return nullptr;
}
inline void operator delete (void* p, mem_allocator& alloc) {
	alloc.deallocate(p);
}
template<class T> inline cause::t new_destroy(T* p, mem_allocator& alloc) {
	p->~T();
	return alloc.deallocate(p);
}

mem_allocator& shared_mem();


#endif  // include guard

