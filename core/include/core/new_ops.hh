/// @file   core/new_ops.hh
//
// (C) 2010,2013-2014 KATO Takeshi
//

#ifndef CORE_NEW_OPS_HH_
#define CORE_NEW_OPS_HH_

#include <core/basic.hh>


class mem_allocator
{
public:
	struct interfaces
	{
		void init() {
			Allocate = nofunc_Allocate;
			Deallocate = nofunc_Deallocate;
			GetSize = nofunc_GetSize;
		}

		typedef cause::pair<void*> (*AllocateIF)(
		    mem_allocator* x, uptr bytes);
		AllocateIF Allocate;

		typedef cause::t (*DeallocateIF)(
		    mem_allocator* x, void* p);
		DeallocateIF Deallocate;

		typedef cause::pair<uptr> (*GetSizeIF)(
		    mem_allocator* x, void* p);
		GetSizeIF GetSize;
	};

	mem_allocator() {}
	mem_allocator(const interfaces* _ifs) : ifs(_ifs) {}

public:
	template<class T>
	static cause::pair<void*> call_on_Allocate(mem_allocator* x, uptr bytes)
	{
		return static_cast<T*>(x)->on_Allocate(bytes);
	}
	static cause::pair<void*> nofunc_Allocate(mem_allocator*, uptr)
	{
		return null_pair(cause::NOFUNC);
	}

	template<class T>
	static cause::t call_on_Deallocate(mem_allocator* x, void* p)
	{
		return static_cast<T*>(x)->on_Deallocate(p);
	}
	static cause::t nofunc_Deallocate(mem_allocator*, void*)
	{
		return cause::NOFUNC;
	}

	template<class T>
	static cause::pair<uptr> call_on_GetSize(mem_allocator* x, void* p)
	{
		return static_cast<T*>(x)->on_GetSize(p);
	}
	static cause::pair<uptr> nofunc_GetSize(mem_allocator*, void*)
	{
		return zero_pair(cause::NOFUNC);
	}

public:
	cause::pair<void*> allocate(uptr bytes)
	{
		return ifs->Allocate(this, bytes);
	}

	cause::t deallocate(void* p)
	{
		return ifs->Deallocate(this, p);
	}

	cause::pair<uptr> get_size(void* p)
	{
		return ifs->GetSize(this, p);
	}

protected:
	const interfaces* ifs;
};

inline void* operator new  (uptr, void* ptr) throw() { return ptr; }
inline void* operator new[](uptr, void* ptr) throw() { return ptr; }

inline void operator delete  (void*, void*) {}
inline void operator delete[](void*, void*) {}

inline void* operator new (uptr size, mem_allocator& alloc) throw()
{
	auto r = alloc.allocate(size);
	if (is_ok(r))
		return r.get_data();
	else
		return nullptr;
}
inline void operator delete (void* p, mem_allocator& alloc)
{
	alloc.deallocate(p);
}
inline void* operator new[] (uptr size, mem_allocator& alloc) throw()
{
	auto r = alloc.allocate(size);
	if (is_ok(r))
		return r.get_data();
	else
		return nullptr;
}
inline void operator delete[] (void* p, mem_allocator& alloc)
{
	alloc.deallocate(p);
}
template<class T> inline cause::t new_destroy(T* p, mem_allocator& alloc)
{
	if (p) {
		p->~T();
		return alloc.deallocate(p);
	}

	return cause::OK;
}

mem_allocator& generic_mem();


#endif  // include guard

