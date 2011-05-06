/// @file  serialout.cc
/// @brief  Output only serial port.
//
// (C) 2010 KATO Takeshi
//

#include "output.hh"

#include "kerninit.hh"
#include "native_ops.hh"
#include "placement_new.hh"
#include "arch.hh"


extern "C" void serial_intr_com1_handler();
extern "C" void serial_intr_com2_handler();

namespace {

// @brief Port offset.
enum {
	RECEIVE_DATA  = 0,
	TRANSMIT_DATA = 0,
	BAUDRATE_LSB  = 0,
	BAUDRATE_MSB  = 1,
	INTR_ENABLE   = 1,
	INTR_ID       = 2,
	FIFO_CTRL     = 2,
	LINE_CTRL     = 3,
	MODEM_CTRL    = 4,
	LINE_STATUS   = 5,
	MODEM_STATUS  = 6
};

serial_kout serial_kout_obj;

};

// serial_kout

serial_kout& serial_get_kout()
{
	return serial_kout_obj;
}

void serial_kout_init()
{
	new (&serial_kout_obj) serial_kout;
	serial_kout_obj.init(serial_kout::COM1_BASEPORT);
}

void serial_kout::init(u16 com_base_port)
{
	write_func = default_write_func;
	writec_func = writec;

	base_port = com_base_port;

	native::cli();

	// 通信スピード設定開始
	native::outb(0x80, base_port + LINE_CTRL);

	// 通信スピードの指定 600[bps]
	native::outb(0xc0, base_port + BAUDRATE_LSB);
	native::outb(0x00, base_port + BAUDRATE_MSB);

	// 通信スピード設定終了(送受信開始)
	native::outb(0x03, base_port + LINE_CTRL);

	// 制御ピン設定
	native::outb(0x0b, base_port + MODEM_CTRL);

	// 16550互換モードに設定
	// FIFOが14bytesになる。
	// FIFOをクリアする。
	native::outb(0xcf, base_port + FIFO_CTRL);

	// 割り込みを有効化
	//native::outb(0x03, base_port + INTR_ENABLE);
	// 無効化
	native::outb(0x00, base_port + INTR_ENABLE);

	native::sti();
}

void serial_kout::writec(kernel_log* self, u8 c)
{
	serial_kout* x = (serial_kout*)self;
	for (;;) {
		const u8 r = native::inb(x->base_port + LINE_STATUS);
		if (r & 0x40)
			break;
	}

	native::outb(c, x->base_port + TRANSMIT_DATA);
}


// serial_output
/*
extern "C" void on_serial_intr_com2()
{
	u8 status = native::inb(serial_out[1].base_port + INTR_ID);
	if ((status & 7) == 0x1) {
		// Tx data/FIFO empty
		//serial_out[1].out_buf_left = serial_output::OUT_BUF_SIZE;
	}
	//kern_output* kout = kern_get_out();
	//kout->put_str("com2intr status:")->put_u8hex(status)->put_c('\n');
}
*/
