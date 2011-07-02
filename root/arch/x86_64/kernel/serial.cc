/// @file  serial.cc
/// @brief serial port.
//
// (C) 2011 KATO Takeshi
//

#include "core_class.hh"
#include "event.hh"
#include "fileif.hh"
#include "global_variables.hh"
#include "interrupt_control.hh"
#include "irq_control.hh"
#include "memory_allocate.hh"
#include "native_ops.hh"
#include "placement_new.hh"

#include "output.hh"
void lapic_dump();

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

enum {
	DEVICE_TXBUF_SIZE = 16,
};

class serial_ctrl : public file_interface
{
	const u16 base_port;
	const u16 irq_num;

	u8 output_buf[2048];
	// 次に push するインデックス
	u32 output_push_index;
	// 最後に pop したインデックス
	u32 output_pop_index;
	bool output_fifo_empty;

	event_item intr_event;
	volatile bool intr_posted;

public:
	/// @todo: do not use global var.
	static file_ops serial_ops;

public:
	serial_ctrl(u16 base_port_, u16 irq_num_);
	cause::stype configure();

private:
	bool is_output_buf_empty() const {
		if (output_push_index == 0)
			return output_pop_index == (sizeof output_buf - 1);
		else
			return output_push_index == (output_pop_index + 1);
	}
	bool is_txfifo_empty() const;
	cause::stype write_(const void* data, uptr size, uptr offset);

	void on_intr_event();
	static void on_intr_event_(void* param);
	void post_intr_event();
	static void intr_handler(void* param);

	void transmit();

public:
	static cause::stype write(
	    file_interface* self, const void* data, uptr size, uptr offset);
	void dump();
};

file_ops serial_ctrl::serial_ops;

serial_ctrl::serial_ctrl(u16 base_port_, u16 irq_num_) :
	base_port(base_port_),
	irq_num(irq_num_),
	output_push_index(1),
	output_pop_index(0),
	output_fifo_empty(true)
{
	ops = &serial_ops;
}

cause::stype serial_ctrl::configure()
{
	u32 vec;
	arch::irq_interrupt_map(4, &vec);

	static interrupt_handler ih;
	ih.param = this;
	ih.handler = intr_handler;
	global_variable::gv.core->intr_ctrl.add_handler(vec, &ih);

	intr_event.handler = on_intr_event_;
	intr_event.param = this;
	intr_posted = false;

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
	native::outb(0x03, base_port + INTR_ENABLE);
	// 無効化
	//native::outb(0x00, base_port + INTR_ENABLE);

	return cause::OK;
}

bool serial_ctrl::is_txfifo_empty() const
{
	const u8 line_status = native::inb(base_port + LINE_STATUS);

	return (line_status & 0x20) != 0;
}

// TODO: exclusive
cause::stype serial_ctrl::write_(
    const void* data, uptr size, uptr offset)
{
	offset = offset;

	const char* src = reinterpret_cast<const char*>(data);

	u32 push = output_push_index;

	for (u32 i = 0; i < size; ++i) {
		if (push == output_pop_index)
			break;

		output_buf[push] = src[i];

		++push;
		if (push >= sizeof output_buf)
			push = 0;
	}

	output_push_index = push;

	if (output_fifo_empty) {
		output_fifo_empty = false;
		transmit();
	}

	return cause::OK;
}

cause::stype serial_ctrl::write(
    file_interface* self, const void* data, uptr size, uptr offset)
{
	serial_ctrl* serial = reinterpret_cast<serial_ctrl*>(self);

	return serial->write_(data, size, offset);
};

void serial_ctrl::on_intr_event()
{
	const u8 intr_id = native::inb(base_port + INTR_ID);

	switch (intr_id & 0x0e) {
	// priority order
	case 0x6:  // rx line status
		// fall through
	case 0x4:  // rx fifo trigger
		// fall through
	case 0xc:  // rx fifo time out

		// 送信バッファが空になったときの割り込みは優先度が低いので
		// ここで確認しておく。
		if (is_txfifo_empty())
			transmit();

		break;

	case 0x2:  // tx fifo empty
		transmit();
		// fall through
	case 0x0:  // modem status
		;
		// fall through
	}
}

void serial_ctrl::on_intr_event_(void* param)
{
	serial_ctrl* serial = reinterpret_cast<serial_ctrl*>(param);
	serial->intr_posted = false;
	serial->on_intr_event();
}

void serial_ctrl::post_intr_event()
{
	// TODO: exclusive
	if (intr_posted)
		return;

	intr_posted = true;
	post_event(&intr_event);
}

/// 割り込み発生時に呼ばれる。
void serial_ctrl::intr_handler(void* param)
{
	serial_ctrl* serial = reinterpret_cast<serial_ctrl*>(param);
	serial->post_intr_event();
}

/// デバイスのFIFOのサイズだけ送信する。
/// デバイスのFIFOが空になったら呼ばれる。
//
/// TODO: exclusive
void serial_ctrl::transmit()
{
	u32 pop = output_pop_index;

	if (is_output_buf_empty()) {
		output_fifo_empty = true;
		return;
	}

	for (u32 i = 0; i < DEVICE_TXBUF_SIZE; ++i) {
		++pop;
		if (pop >= sizeof output_buf)
			pop = 0;

		if (pop == output_push_index) {
			--pop;
			break;
		}

		native::outb(output_buf[pop], base_port + TRANSMIT_DATA);

	}

	output_pop_index = pop;
}

void serial_ctrl::dump()
{
	kern_output* kout = kern_get_out();
	kout->put_str("serial_dump:");
	kout->put_str("output_push_index:")
		->put_udec(output_push_index)
		->put_str(", output_pop_index:")
		->put_udec(output_pop_index)
		->put_str(", output_fifo_empty:")
		->put_udec(output_fifo_empty)
		->put_endl();
}

}

file_interface* create_serial()
{
	///////
	serial_ctrl::serial_ops.write = serial_ctrl::write;
	///////

	void* mem = memory::alloc(sizeof (serial_ctrl));
	serial_ctrl* serial = new (mem) serial_ctrl(0x03f8, 4);

	serial->configure();

	return serial;
}

void serial_dump(void* p)
{
	serial_ctrl* s = (serial_ctrl*)p;
	s->dump();
}

