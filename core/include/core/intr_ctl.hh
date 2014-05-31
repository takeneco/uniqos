/// @file  intr_ctl.hh
//
// (C) 2011-2014 KATO Takeshi
//

#ifndef CORE_INCLUDE_CORE_INTR_CTL_HH_
#define CORE_INCLUDE_CORE_INTR_CTL_HH_

#include <arch.hh>
#include "arch_specs.hh"
#include <core/chain.hh>


/// @brief 割り込み発生時に呼ばれる関数。
//
/// このクラスのインスタンスは割り込みベクタ毎に生成する必要がある。
class intr_handler
{
	chain_node<intr_handler> _chain_node;

public:
	typedef void (*handler_type)(intr_handler* handler);

	intr_handler() {}
	intr_handler(handler_type _handler) : handler(_handler) {}

	chain_node<intr_handler>& chain_hook() { return _chain_node; }

	handler_type handler;
};

template <class T>
class intr_handler_with : public intr_handler
{
public:
	intr_handler_with()
	{}
	explicit intr_handler_with(handler_type _handler) :
		intr_handler(_handler)
	{}
	intr_handler_with(handler_type _handler, T _data) :
		intr_handler(_handler),
		data(_data)
	{}

	T data;
};


/// @note  must init by constructor.
class intr_ctl
{
	typedef
	    chain<intr_handler, &intr_handler::chain_hook>
	    intr_handler_chain;

	typedef void (* post_intr_handler)();

	struct intr_task
	{
		intr_handler_chain handler_chain;
		post_intr_handler post_handler;

		intr_task() : post_handler(0) {}
	};

	intr_task handler_table[arch::INTR_COUNT];

public:
	/// @internal  initialize handler_table[]
	intr_ctl() {}

	cause::t init();
	cause::t install_handler(arch::intr_id vec, intr_handler* h);
	cause::t set_post_handler(arch::intr_id vec, post_intr_handler h);
	void call_interrupt(u32 vector);
};


#endif  // Include guard

