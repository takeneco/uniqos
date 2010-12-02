/// @file  bitmap.hh
/// @brief Bitmap structure.
//
// (C) 2010 Kato Takeshi
//

#ifndef BITMAP_HH_
#define BITMAP_HH_

#include "arch.hh"
#include "native_ops.hh"


template<class bittype_>
class bitmap
{
	bittype_ map;

public:
	enum {
		BITS = arch::BITS_PER_BYTE * sizeof map,
	};

	// Cast to bittype_
	static bittype_ b(bittype_ x) { return x; }

public:
	bitmap() {}
	explicit bitmap(bittype_ x) : map(x) {}

	bittype_ get_raw() const { return map; }

	void set_true(int i)  { map |= b(1) << i; }
	void set_false(int i) { map &= ~(b(1) << i); }
	void set_true_all()   { map = ~b(0); }
	void set_false_all()  { map = b(0); }

	bool is_true(int i) { return map == b(1) << i; }
	bool is_false(int i) { return map != b(1) << i; }
	bool is_true_all() { return map == ~b(0); }
	bool is_false_all() { return map == b(0); }

	int search_true() const {
		const int r = static_cast<int>(bitscan_forward(map));
		// assert(r >= 0)
		return r;
	}
	int search_false() const {
		const int r = static_cast<int>(bitscan_forward(~map));
		// assert(r >= 0)
		return r;
	}
};


#endif  // Include guards.
