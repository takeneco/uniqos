/// @file  basic_defs.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_BASIC_DEFS_HH_
#define INCLUDE_BASIC_DEFS_HH_


/// Disable default copy constructor and operator=.
/// class example
/// {
///     DISALLOW_COPY_AND_ASSIGN(example);
/// public:
/// };
#define DISALLOW_COPY_AND_ASSIGN(T)  T(const T&);void operator=(const T&)


#define num_of_array(array) (sizeof array / sizeof array[0])


#ifdef __GNUC__
# define LIKELY(x)   __builtin_expect(!!(x), 1)
# define UNLIKELY(x) __builtin_expect(x, 0)
#else  // !__GNUC__
# define LIKELY(x)   (x)
# define UNLIKELY(x) (x)
#endif  // __GNUC__

#ifdef ARCH_BE
# define ARCH_IS_BE_LE(BE_CODE, LE_CODE)  (BE_CODE)
#else   // ARCH_BE
# define ARCH_IS_BE_LE(BE_CODE, LE_CODE)  (LE_CODE)
#endif  // ARCH_BE

template<class xint> bool test_overlap(
    xint a1, xint a2, xint b1, xint b2, xint* r1, xint* r2)
{
	const xint p1 = max(a1, b1);
	const xint p2 = min(a2, b2);
	if (p1 > p2)
		return false;

	*r1 = p1;
	*r2 = p2;
	return true;
}


#endif  // include guard

