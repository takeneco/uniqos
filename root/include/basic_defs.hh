/// @file  basic_defs.hh
//
// (C) 2012 KATO Takeshi
//

#ifndef BASIC_DEFS_HH_
#define BASIC_DEFS_HH_


/// Disable default copy constructor and operator=.
/// class example
/// {
///     DISALLOW_COPY_AND_ASSIGN(example);
/// public:
/// };
#define DISALLOW_COPY_AND_ASSIGN(T)  T(const T&);void operator=(const T&)


#ifdef __GNUC__
# define LIKELY(x)   __builtin_expect(!!(x), 1)
# define UNLIKELY(x) __builtin_expect(x, 0)
#else  // !__GNUC__
# define LIKELY(x)   x
# define UNLIKELY(x) x
#endif  // __GNUC__


#endif  // include guards
