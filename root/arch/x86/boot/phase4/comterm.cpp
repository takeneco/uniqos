/**
 * @file    arch/x86/boot/phase4/comterm.c
 * @version 0.0.1
 * @date    2009-07-06
 * @author  Kato.T
 *
 * 簡単なシリアルコンソール出力。
 */
// (C) Kato.T 2009

#include <cstddef>
#include "phase4.hpp"


/**
 * com_term クラスを初期化する。
 *
 * @param port ベースポート。
 */
void com_term::init(_u16 port)
{
	base_port = port;
	data_head = data_tail = 0;
}

/**
 * COM へ１文字出力する。
 *
 * @param ch 出力する文字。
 */
void com_term::putc(char ch)
{
}

