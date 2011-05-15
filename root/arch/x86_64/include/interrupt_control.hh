/// @file  interrupt_control.hh
//
// (C) 2011 KATO Takeshi
//

#ifndef ARCH_X86_64_INCLUDE_INTERRUPT_CONTROL_HH_
#define ARCH_X86_64_INCLUDE_INTERRUPT_CONTROL_HH_


class interrupt_handler
{
	chain_link<interrupt_handler> chain_link_;

public:
	chain_link<interrupt_handler>& chain_hook() { return chain_link_; }

	void* param;
	void (*handler)(void* param);
};

class interrupt_control
{
	chain<interrupt_handler, &interrupt_handler::chain_hook>
	    handler_table[0x40];

public:
	cause::stype init();
	cause::stype add_handler(u8 vec, interrupt_handler* h);
};


#endif  // include guard

