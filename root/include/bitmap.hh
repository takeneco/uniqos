/// @file  int_bitset.hh
/// @brief Bitmap structure.
//
// (C) 2010,2012 KATO Takeshi
//

#ifndef INT_BITSET_HH_
#define INT_BITSET_HH_

#include "arch.hh"
#include <bitops.hh>


template<class TYPE>
class int_bitset
{
	TYPE field;

public:
	enum {
		BITS = arch::BITS_PER_BYTE * sizeof (TYPE),
	};

	// Cast to TYPE
	static TYPE b(TYPE x) { return x; }

public:
	int_bitset() {}
	explicit int_bitset(TYPE x) : field(x) {}

	void set_raw(TYPE x) { field = x; }
	TYPE get_raw() const { return field; }

	void set_true(int i)  { field |= b(1) << i; }
	void set_false(int i) { field &= ~(b(1) << i); }
	void set_true_all()   { field = ~b(0); }
	void set_false_all()  { field = b(0); }

	bool is_true(int i) { return field & (b(1) << i); }
	bool is_false(int i) { return !is_true(i); }
	bool is_true_all() { return field == ~b(0); }
	bool is_false_all() { return field == b(0); }

	int search_true() const {
		return arch::find_first_setbit(field);
	}
	int search_false() const {
		return arch::find_first_setbit(~field);
	}
};


#endif  // include guard

