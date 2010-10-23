/// @author  KATO Takeshi
/// @brief   Bitmap structure.
//
// (C) 2010 Kato Takeshi

#ifndef BITMAP_HH_
#define BITMAP_HH_

#include "arch.hh"
#include "native.hh"


template<class bittype_>
class bitmap
{
	bittype_ map;

public:
	enum {
		BITS = arch::BITS_PER_BYTE * sizeof map,
	};

public:
	bittype_ get_raw() const { return map; }

	void set_true(int i)  { map |= static_cast<bittype_>(1) << i; }
	void set_false(int i) { map &= ~(static_cast<bittype_>(1) << i); }
	void set_true_all()   { map = ~static_cast<bittype_>(0); }
	void set_false_all()  { map = static_cast<bittype_>(0); }

	bool is_all_true() { return map == ~static_cast<bittype_>(0); }
	bool is_all_false() { return map == static_cast<bittype_>(0); }

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
