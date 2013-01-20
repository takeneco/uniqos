/// @file  pic_dev.hh
//
// (C) 2013 KATO Takeshi
//

#ifndef INCLUDE_PIC_DEV_HH_
#define INCLUDE_PIC_DEV_HH_

#include <arch.hh>


/// @brief PIC(Programmable Interrupt Controller) interface.
class pic_device
{
	DISALLOW_COPY_AND_ASSIGN(pic_device);

	friend class irq_ctl;

public:
	struct operations
	{
		void init();

		typedef cause::type (*enable_op)(
		    pic_device* x, uint irq, arch::intr_id vec);
		enable_op enable;

		typedef cause::type (*disable_op)(
		    pic_device* x, uint irq);
		disable_op disable;

		// End Of Interrupt
		typedef void (*eoi_op)(
		    pic_device* x);
		eoi_op eoi;
	};

	pic_device(operations* _ops) : ops(_ops) {}

	// enable
	template<class T> static cause::type call_on_pic_device_enable(
	    pic_device* x, uint irq, arch::intr_id vec) {
		return static_cast<T*>(x)->
		    on_pic_device_enable(irq, vec);
	}
	static cause::type nofunc_pic_device_enable(
	    pic_device*, uint, arch::intr_id) {
		return cause::NOFUNC;
	}

	// disable
	template<class T> static cause::type call_on_pic_device_disable(
	    pic_device* x, uint irq) {
		return static_cast<T*>(x)->
		    on_pic_device_disable(irq);
	}
	static cause::type nofunc_pic_device_disable(
	    pic_device*, uint) {
		return cause::NOFUNC;
	}

	// eoi
	template<class T> static void call_on_pic_device_eoi(
	    pic_device* x) {
		return static_cast<T*>(x)->
		    on_pic_device_eoi();
	}

private:
	operations* ops;
};


#endif  // include guard

