/// @file  irq_src.hh
//
// (C) 2013 KATO Takeshi
//

#ifndef INCLUDE_IRQ_SRC_HH_
#define INCLUDE_IRQ_SRC_HH_

#include <arch.hh>


class irq_source
{
	DISALLOW_COPY_AND_ASSIGN(irq_source);

public:
	struct operations
	{
		void init();

		typedef cause::type (*enable_op)(
		    irq_source* x, uint irq, arch::intr_id vec);
		enable_op enable;

		typedef cause::type (*disable_op)(
		    irq_source* x, uint irq);
		disable_op disable;

		// End Of Interrupt
		typedef void (*eoi_op)(
		    irq_source* x);
		eoi_op eoi;
	};

	// enable
	template<class T> static cause::type call_on_irq_source_enable(
	    irq_source* x, uint irq, arch::intr_id vec) {
		return static_cast<T*>(x)->
		    on_irq_source_enable(irq, vec);
	}
	static cause::type nofunc_irq_source_enable(
	    irq_source*, uint, arch::intr_id) {
		return cause::NOFUNC;
	}

	// disable
	template<class T> static cause::type call_on_irq_source_disable(
	    irq_source* x) {
		return static_cast<T*>(x)->
		    on_irq_source_disable(x);
	}
	static cause::type nofunc_irq_source_disable(
	    irq_source*) {
		return cause::NOFUNC;
	}

	// eoi
	template<class T> static void call_on_irq_source_eoi(
	    irq_source*) {
		return static_cast<T*>(x)->
		    on_irq_source_eoi(x);
	}
};


#endif  // include guard

