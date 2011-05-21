/// @file  interrupt_control.hh
//
// (C) 2011 KATO Takeshi
//

#ifndef INCLUDE_INTERRUPT_CONTROL_HH_
#define INCLUDE_INTERRUPT_CONTROL_HH_

#include "arch_specs.hh"
#include "chain.hh"


/// @brief 割り込み発生時に呼ばれる関数。
//
/// このクラスのインスタンスは割り込みベクタ毎に生成する必要がある。
class interrupt_handler
{
	chain_link<interrupt_handler> chain_link_;

public:
	chain_link<interrupt_handler>& chain_hook() { return chain_link_; }

	void* param;
	void (*handler)(void* param);
};

/// @note  must init by constructor.
class interrupt_control
{
	typedef
	    chain<interrupt_handler, &interrupt_handler::chain_hook>
	    intr_handler_chain;

	intr_handler_chain handler_table[arch::INTR_COUNT];

public:
	/// @internal  initialize handler_table[]
	interrupt_control() {}

	cause::stype init();
	cause::stype add_handler(u8 vec, interrupt_handler* h);
	void call_interrupt(u32 vector);
};

void on_interrupt(arch::intr_vec index);


#endif  // include guard

