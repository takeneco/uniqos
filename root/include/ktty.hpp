// @file    include/ktty.hpp
// @author  Kato Takeshi
// @brief   カーネルメッセージの出力先。
//
// (C) Kato Takeshi 2008-2010

#ifndef KTTY_HH_
#define KTTY_HH_

#include <cstddef>
#include "btypes.hh"


class ktty
{
	void putux(_u64 n, _ucpu bits);
public:
	ktty() {}
	virtual ktty* putc(char c) =0;
	ktty* puts(const char* str);
	ktty* putsd(int n);
	ktty* put8x(_u8 n);
	ktty* put16x(_u16 n);
	ktty* put32x(_u32 n);
	ktty* put64x(_u64 n);
};

ktty* create_ktty();

#endif  // Include guard.
