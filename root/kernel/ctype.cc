/// @file  ctype.cc
//
// (C) 2012 KATO Takeshi
//

#include <ctype.hh>
#include <ctype.h>


namespace ctype {

const u8 _masktbl[] = {
     0,     0,     0,     0,     0,     0,     0,     0,  // 0x00-0x07
     0,    _s,    _s,    _s,    _s,    _s,     0,     0,  // 0x08-0x0f
     0,     0,     0,     0,     0,     0,     0,     0,  // 0x10-0x17
     0,     0,     0,     0,     0,     0,     0,     0,  // 0x18-0x1f
    _s,    _p,    _p,    _p,    _p,    _p,    _p,    _p,  // 0x20-0x27
    _p,    _p,    _p,    _p,    _p,    _p,    _p,    _p,  // 0x28-0x2f
    _d,    _d,    _d,    _d,    _d,    _d,    _d,    _d,  // 0x30-0x37
    _d,    _d,    _p,    _p,    _p,    _p,    _p,    _p,  // 0x38-0x3f
    _p, _h|_u, _h|_u, _h|_u, _h|_u, _h|_u, _h|_u,    _u,  // 0x40-0x47
    _u,    _u,    _u,    _u,    _u,    _u,    _u,    _u,  // 0x48-0x4f
    _u,    _u,    _u,    _u,    _u,    _u,    _u,    _u,  // 0x50-0x57
    _u,    _u,    _u,    _p,    _p,    _p,    _p,    _p,  // 0x58-0x5f
    _p, _h|_l, _h|_l, _h|_l, _h|_l, _h|_l, _h|_l,    _l,  // 0x60-0x67
    _l,    _l,    _l,    _l,    _l,    _l,    _l,    _l,  // 0x68-0x6f
    _l,    _l,    _l,    _l,    _l,    _l,    _l,    _l,  // 0x70-0x77
    _l,    _l,    _l,    _p,    _p,    _p,    _p,     0,  // 0x78-0x7f
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,
};

}  // namespace ctype

extern "C" {

int isalpha(int c)  { return ctype::is_alpha(c);  }
int isdigit(int c)  { return ctype::is_digit(c);  }
int isprint(int c)  { return ctype::is_print(c);  }
int isspace(int c)  { return ctype::is_space(c);  }
int isupper(int c)  { return ctype::is_upper(c);  }
int isxdigit(int c) { return ctype::is_xdigit(c); }

}  // extern "C"

