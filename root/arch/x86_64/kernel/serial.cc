/// @file  serial.cc
/// @brief serial port.
//
// (C) 2011 KATO Takeshi
//

#include "core_class.hh"
#include "chain.hh"
#include "event.hh"
#include "fileif.hh"
#include "global_vars.hh"
#include "interrupt_control.hh"
#include "irq_control.hh"
#include "memcache.hh"
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


class buf_entry
{
	struct {
		bichain_link<buf_entry> chain_link;
	} f;

	u8* buf() { return reinterpret_cast<u8*>(this + 1); }

public:
	enum {
		SIZE = arch::page::L1_SIZE,
		BUF_SIZE = SIZE - sizeof f,
	};

public:
	bichain_link<buf_entry>& chain_hook() { return f.chain_link; }

	void write(u32 i, u8 c) { buf()[i] = c; }
	u8 read(u32 i) { return buf()[i]; }
};


class serial_ctrl : public file_interface
{
	const u16 base_port;
	const u16 irq_num;

	bidechain<buf_entry, &buf_entry::chain_hook> buf_queue;

	/// バッファに書くときは buf_queue->head() の next_write に書く。
	u32 next_write;

	/// バッファを読むときは buf_queue->tail() の next_read から読む。
	u32 next_read;

	mem_cache* buf_mc;

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
	bool buf_is_empty() const {
		const buf_entry* h = buf_queue.head();
		return (h == 0) ||
		       (next_write == next_read && buf_queue.next(h) == 0);
	}
	buf_entry* buf_append() {
		buf_entry* buf = new (buf_mc->alloc()) buf_entry;
		if (buf == 0)
			return 0;
		buf_queue.insert_head(buf);
		next_write = 0;
		return buf;
	}
	bool is_txfifo_empty() const;
	cause::stype write_(const void* data, uptr size);

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
	next_read(0),
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
	global_vars::gv.core->intr_ctrl.add_handler(vec, &ih);

	intr_event.handler = on_intr_event_;
	intr_event.param = this;
	intr_posted = false;

	// 通信スピード設定開始
	native::outb(0x80, base_port + LINE_CTRL);
/*
	// 通信スピードの指定 600[bps]
	native::outb(0xc0, base_port + BAUDRATE_LSB);
	native::outb(0x00, base_port + BAUDRATE_MSB);
*/
	// fastest
	native::outb(0x01, base_port + BAUDRATE_LSB);
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

	buf_mc = shared_mem_cache(buf_entry::SIZE);

	return cause::OK;
}

bool serial_ctrl::is_txfifo_empty() const
{
	const u8 line_status = native::inb(base_port + LINE_STATUS);

	return (line_status & 0x20) != 0;
}

// TODO: exclusive
cause::stype serial_ctrl::write_(const void* data, uptr size)
{
	const char* src = reinterpret_cast<const char*>(data);

	buf_entry* buf = buf_queue.head();
	if (buf == 0) {
		buf = buf_append();
		if (buf == 0)
			return cause::NO_MEMORY;
	}

	for (uptr i = 0; i < size; ++i) {
		if (next_write >= buf_entry::BUF_SIZE) {
			buf = buf_append();
			if (buf == 0)
				return cause::NO_MEMORY;
		}
		buf->write(next_write++, src[i]);
	}

	if (output_fifo_empty) {
		output_fifo_empty = false;
		transmit();
	}

	return cause::OK;
}

cause::stype serial_ctrl::write(
    file_interface* self, const void* data, uptr size, uptr offset)
{
	offset=offset;

	serial_ctrl* serial = reinterpret_cast<serial_ctrl*>(self);

	return serial->write_(data, size);
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
	if (buf_is_empty()) {
		output_fifo_empty = true;
		return;
	}

	buf_entry* buf = buf_queue.tail();
	bool buf_is_last = buf == buf_queue.head();

	for (u32 i = 0; i < DEVICE_TXBUF_SIZE; ++i) {
		if (buf == 0)
			break;
		if (next_read == buf_entry::BUF_SIZE) {
			buf_mc->free(buf_queue.remove_tail());
			buf = buf_queue.tail();
			buf_is_last = buf == buf_queue.head();
			next_read = 0;
			if (buf == 0)
				break;
		}
		if (buf_is_last && next_write == next_read)
			break;
		native::outb(buf->read(next_read++), base_port + TRANSMIT_DATA);
	}
}

void serial_ctrl::dump()
{
	kern_output* kout = kern_get_out();
	kout->put_str("serial::dump()")->put_endl();
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

