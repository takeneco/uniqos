/**
 * @file    include/ktty.hpp
 * @version 0.0.9
 * @date    2009-08-02
 * @author  Kato.T
 * @brief   カーネルメッセージの出力先。
 */
// (C) Kato.T 2008-2009

#ifndef KTTY_HPP
#define KTTY_HPP

#include <cstddef>
#include "btypes.hpp"


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

#endif  // KTTY_HPP
