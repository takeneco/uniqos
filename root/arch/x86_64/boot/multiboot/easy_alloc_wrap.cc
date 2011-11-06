/// @file   easy_alloc_wrap.cc
/// @brief  memory allocation implement wrapper.
//
// (C) 2010-2011 KATO Takeshi
//

#include "log.hh"
#include "easy_alloc.hh"
#include "misc.hh"
#include "placement_new.hh"


namespace {

typedef easy_alloc<256> allocator;

// コンストラクタを呼ばせない。
uptr mem_buf_[sizeof (allocator) / sizeof (uptr)];
inline allocator* get_alloc() {
	return reinterpret_cast<allocator*>(mem_buf_);
}

}  // namespace


/// @brief  Initialize alloc.
void mem_init()
{
	new (get_alloc()) allocator;
	get_alloc()->init();
}

/// @brief  空きメモリを追加する。
/// @param[in] avoid  カーネルのために空けておきたいメモリなら true。
void mem_add(uptr adr, uptr bytes, bool avoid)
{
	get_alloc()->add_free(adr, bytes, avoid);
}

/// @brief  Allocate memory.
/// @param[in] bytes  required memory size.
/// @param[in] align  Memory aliment. Must to be 2^n.
///     EX: If align == 256, then return address is "0x....00".
/// @param[in] forget  Forget this memory
/// @return If succeeds, this func returns allocated memory address ptr.
/// @return If fails, this func returns 0.
/// @param[in] forget true ならkernelに jmp した直後に開放される。
///                        true なら avoid の領域を割り当てる。
///  - kernel に jmp した後も使い続けるなら forget = false
///  - kernel に jmp する前に明示的に開放するなら forget = false
///  - forget = true のときは kernel に jmp するときにメモリを使用中だという
///    情報を残さないだけ。
void* mem_alloc(uptr bytes, uptr align, bool forget)
{
	return get_alloc()->alloc(bytes, align, forget);
}

/// @brief  Free memory.
/// @param[in] p  Ptr to memory free.
void mem_free(void* p)
{
	get_alloc()->free(p);
}

bool mem_reserve(uptr adr, uptr bytes, bool forget)
{
	return get_alloc()->reserve(adr, bytes, forget);
}

void mem_alloc_info(mem_enum* me)
{
	me->avoid = false;
	me->entry = get_alloc()->alloc_info();
}

bool mem_alloc_info_next(mem_enum* me, uptr* adr, uptr* bytes)
{
	const void* next;

	if (me->avoid == false) {
		next = get_alloc()->alloc_info_next(me->entry, adr, bytes);
		if (next == 0) {
			next = get_alloc()->avoid_alloc_info();
			me->avoid = true;
		}
	}
	else {
		next =
		    get_alloc()->avoid_alloc_info_next(me->entry, adr, bytes);
	}

	me->entry = next;

	return next != 0;
}
/*
void mem_dump()
{
	get_alloc()->debug_dump(log());
}
*/

namespace arch {
namespace page {

cause::stype alloc(TYPE page_type, uptr* padr)
{
	const u32 size = inline_page_size(page_type);
	void* p = mem_alloc(size, size, false);
	if (!p)
		return cause::NO_MEMORY;

	*padr = reinterpret_cast<uptr>(p);
	return cause::OK;
}

cause::stype free(TYPE, uptr padr)
{
	mem_free(reinterpret_cast<void*>(padr));
	return cause::OK;
}

}  // namespace page
}  // namespace arch

