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

	enum {
		ZERO = static_cast<bittype_>(0),
		ONE = static_cast<bittype_>(1),
	};

public:
	enum {
		BITS = arch::BITS_PER_BYTE * sizeof map,
	};

public:
	void set_true(int i)  { map |= ONE << i; }
	void set_false(int i) { map &= ~(ONE << i); }
	void set_true_all()   { map = ~ZERO; }
	void set_false_all()  { map = ZERO; }

	bool is_all_true() { return map == ~ZERO; }
	bool is_all_false() { return map == ZERO; }

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
