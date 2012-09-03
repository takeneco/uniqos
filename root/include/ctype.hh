/// @file  ctype.hh
/// @brief used by ACPICA
//
// (C) 2012 KATO Takeshi
//

#ifndef INCLUDE_CTYPE_HH_
#define INCLUDE_CTYPE_HH_

#include <basic.hh>


namespace ctype {

enum {
	//_c = 0x01,  ///< Control
	_u = 0x02,  ///< Upper
	_l = 0x04,  ///< Lower
	_d = 0x08,  ///< Digit
	_s = 0x10,  ///< Space
	_p = 0x20,  ///< others Printable

	_h = 0x40,  ///< Hex digit
};

extern const u8 _masktbl[];

inline int is_upper(u8 c) { return _masktbl[c] & (_u);             }
inline int is_lower(u8 c) { return _masktbl[c] & (_l);             }
inline int is_digit(u8 c) { return _masktbl[c] & (_d);             }
inline int is_space(u8 c) { return _masktbl[c] & (_s);             }
inline int is_alpha(u8 c) { return _masktbl[c] & (_u|_l);          }
inline int is_print(u8 c) { return _masktbl[c] & (_u|_l|_d|_s|_p); }
inline int is_xdigit(u8 c) { return _masktbl[c] & (_h);            }

}  // namespace ctype


#endif  // INCLUDE_CTYPE_HH_

