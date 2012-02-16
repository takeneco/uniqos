/// @file  serial.cc
/// @brief serial port.
//
// (C) 2011-2012 KATO Takeshi
//

#include "core_class.hh"
#include "chain.hh"
#include "event.hh"
#include "file.hh"
#include "global_vars.hh"
#include "interrupt_control.hh"
#include "irq_control.hh"
#include "memcache.hh"
#include "memory_allocate.hh"
#include "native_ops.hh"
#include "placement_new.hh"
#include <processor.hh>


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
	struct info {
		info()
		:   client_thread(0),
		    buf_size(buf_entry::BUF_SIZE)
		{}

		bichain_node<buf_entry> chain_node;
		thread* client_thread;
		uptr buf_size;
	} f;

	u8* buf() { return reinterpret_cast<u8*>(this + 1); }

public:
	enum {
		SIZE = arch::page::L1_SIZE,
		BUF_SIZE = SIZE - sizeof (info),
	};

public:
	bichain_node<buf_entry>& chain_hook() { return f.chain_node; }

	void write(u32 i, u8 c) { buf()[i] = c; }
	u8 read(u32 i) { return buf()[i]; }

	void set_client(thread* t) { f.client_thread = t; }
	thread* get_client() { return f.client_thread; }

	void set_bufsize(uptr bs) { f.buf_size = bs; }
	uptr get_bufsize() const { return f.buf_size; }
};


class serial_ctrl : public file
{
	const u16 base_port;
	const u16 irq_num;

	bibochain<buf_entry, &buf_entry::chain_hook> buf_queue;

	/// バッファに書くときは buf_queue->head() の next_write に書く。
	u32 next_write;

	/// バッファを読むときは buf_queue->tail() の next_read から読む。
	u32 next_read;

	mempool* buf_mp;

	bool output_fifo_empty;
	int tx_fifo_queued;

	event_item write_event;
	event_item intr_event;
	bool write_posted;
	volatile bool intr_posted;
	volatile bool intr_pending;

public:
	/// @todo: do not use global var.
	static file::operations serial_ops;

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
		buf_entry* buf = new (buf_mp->alloc()) buf_entry;
		if (buf == 0)
			return 0;
		buf_queue.insert_head(buf);
		next_write = 0;
		return buf;
	}
	bool is_txfifo_empty() const;
	cause::stype write(const iovec* iov, int iov_cnt, uptr* bytes);

	void on_write_event();
	static void on_write_event_(void* param);
	void post_write_event();

	void on_intr_event();
	static void on_intr_event_(void* param);
	void post_intr_event();
	static void intr_handler(void* param);

	void transmit();

public:
	static cause::stype op_write(
	    file* self, const iovec* iov, int iov_cnt, uptr* bytes);
	void dump();
};

file::operations serial_ctrl::serial_ops;

serial_ctrl::serial_ctrl(u16 base_port_, u16 irq_num_) :
	base_port(base_port_),
	irq_num(irq_num_),
	next_read(0),
	output_fifo_empty(true),
	tx_fifo_queued(0),
	intr_pending(false)
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
	global_vars::gv.intr_ctl_obj->add_handler(vec, &ih);

	write_event.handler = on_write_event_;
	write_event.param = this;
	write_posted = false;

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

	buf_mp = mempool_create_shared(buf_entry::SIZE);

	return cause::OK;
}

bool serial_ctrl::is_txfifo_empty() const
{
	const u8 line_status = native::inb(base_port + LINE_STATUS);

	return (line_status & 0x20) != 0;
}

// TODO: exclusive
cause::stype serial_ctrl::write(const iovec* iov, int iov_cnt, uptr* bytes)
{
	uptr total = 0;

	iovec_iterator iov_itr(iov, iov_cnt);

	buf_entry* buf = buf_queue.head();
	if (buf == 0) {
		buf = buf_append();
		if (buf == 0)
			return cause::NOMEM;
	}

	for (;;) {
		const u8* c = iov_itr.next_u8();
		if (!c)
			break;

		if (next_write >= buf->get_bufsize()) {
			buf = buf_append();
			if (buf == 0)
				return cause::NOMEM;
		}
		buf->write(next_write++, *c);
		++total;
	}

	if (tx_fifo_queued < DEVICE_TXBUF_SIZE) {
		post_write_event();
	}

	*bytes = total;

	thread_ctl& tc =
	    global_vars::gv.logical_cpu_obj_array[0].get_thread_ctl();
	if (sync) {
		buf->set_bufsize(next_write);
		buf->set_client(tc.get_running_thread());
	//	tc.sleep_running_thread();
	}

	tc.ready_event_thread();

	return cause::OK;
}

cause::stype serial_ctrl::op_write(
    file* self, const iovec* iov, int iov_cnt, uptr* bytes)
{
	return static_cast<serial_ctrl*>(self)->write(iov, iov_cnt, bytes);
}

void serial_ctrl::on_write_event()
{
	transmit();
}

void serial_ctrl::on_write_event_(void* param)
{
	serial_ctrl* serial = static_cast<serial_ctrl*>(param);
	serial->write_posted = false;
	serial->on_write_event();
}

void serial_ctrl::post_write_event()
{
	if (write_posted) {
		return;
	}

	write_posted = true;

	processor* cpu = arch::get_current_cpu();
	cpu->post_soft_event(&write_event);
	cpu->get_thread_ctl().ready_event_thread();
}

void serial_ctrl::on_intr_event()
{
	intr_pending = false;

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
		if (!is_txfifo_empty())
			break;

		// fall through

	case 0x2:  // tx fifo empty
		tx_fifo_queued = 0;
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
	intr_pending = true;

	// TODO: exclusive
	if (intr_posted)
		return;

	intr_posted = true;

	//post_event(&intr_event);
	processor* cpu = arch::get_current_cpu();
	cpu->post_intr_event(&intr_event);
	cpu->get_thread_ctl().ready_event_thread_in_intr();
}

/// 割り込み発生時に呼ばれる。
void serial_ctrl::intr_handler(void* param)
{
	static_cast<serial_ctrl*>(param)->post_intr_event();
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

	int i;
	for (i = tx_fifo_queued; i < DEVICE_TXBUF_SIZE; ++i) {
		if (intr_pending)
			break;

		if (buf == 0)
			break;
		if (buf_is_last && next_write == next_read)
			break;

		native::outb(buf->read(next_read++), base_port + TRANSMIT_DATA);

		if (next_read == buf->get_bufsize()) {

			thread* client = buf->get_client();
			if (client) {
				thread_ctl& tc = global_vars::
				  gv.logical_cpu_obj_array[0].get_thread_ctl();
				tc.ready_thread(client);
			}

			buf_mp->free(buf_queue.remove_tail());
			buf = buf_queue.tail();
			buf_is_last = buf == buf_queue.head();
			next_read = 0;
		}
	}

	tx_fifo_queued = i;
}

void serial_ctrl::dump()
{
	log()("serial::dump()")();
}

}

namespace {
uptr tmp[(sizeof (serial_ctrl) + sizeof (uptr) - 1) / sizeof (uptr)];
}
file* create_serial()
{
	///////
	serial_ctrl::serial_ops.write = serial_ctrl::op_write;
	///////

	//void* mem = memory::alloc(sizeof (serial_ctrl));
	void* mem = tmp;
	serial_ctrl* serial = new (mem) serial_ctrl(0x03f8, 4);

	serial->configure();

	return serial;
}

void serial_dump(void* p)
{
	serial_ctrl* s = (serial_ctrl*)p;
	s->dump();
}

