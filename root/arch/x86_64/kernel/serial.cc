/// @file  serial.cc
/// @brief serial port.

//  UNIQOS  --  Unique Operating System
//  (C) 2011-2012 KATO Takeshi
//
//  UNIQOS is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  UNIQOS is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <file.hh>
#include <global_vars.hh>
#include <intr_ctl.hh>
#include <irq_ctl.hh>
#include <mempool.hh>
#include <message.hh>
#include <native_ops.hh>
#include <new_ops.hh>
#include <cpu_node.hh>


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
	DISALLOW_COPY_AND_ASSIGN(serial_ctrl);

	friend class file;

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

	typedef message_with<serial_ctrl*> serial_msg;
	serial_msg write_msg;
	serial_msg intr_msg;
	bool write_posted;
	volatile bool intr_posted;
	volatile bool intr_pending;

public:
	/// @todo: do not use global var.
	static file::operations serial_ops;

public:
	serial_ctrl(u16 base_port_, u16 irq_num_);
	cause::type configure();

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
	cause::type on_write(const iovec* iov, int iov_cnt, uptr* bytes);
	cause::type write_buf(iovec_iterator& iov_itr, uptr* bytes);

	void on_write_message();
	static void on_write_message_(message* msg);
	void post_write_message();

	void on_intr_message();
	static void on_intr_message_(message* msg);
	void post_intr_message();
	static void intr_handler(void* param);

	void transmit();

public:
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

cause::type serial_ctrl::configure()
{
	u32 vec;
	arch::irq_interrupt_map(4, &vec);

	static interrupt_handler ih;
	ih.param = this;
	ih.handler = intr_handler;
	global_vars::core.intr_ctl_obj->add_handler(vec, &ih);

	write_msg.handler = on_write_message_;
	write_msg.data = this;
	write_posted = false;

	intr_msg.handler = on_intr_message_;
	intr_msg.data = this;
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

	cause::type r = mempool_acquire_shared(buf_entry::SIZE, &buf_mp);
	if (is_fail(r))
		return r;

	return cause::OK;
}

bool serial_ctrl::is_txfifo_empty() const
{
	const u8 line_status = native::inb(base_port + LINE_STATUS);

	return (line_status & 0x20) != 0;
}

/// @brief  Write to buffer.
/// @param[out] bytes  write bytes.
//
/// 途中で失敗した場合は *bytes に出力したバイト数が返される。
cause::type serial_ctrl::on_write(const iovec* iov, int iov_cnt, uptr* bytes)
{
	iovec_iterator iov_itr(iov, iov_cnt);

	cause::type r;
	{
		preempt_disable_section _pds;

		r = write_buf(iov_itr, bytes);

		if (tx_fifo_queued < DEVICE_TXBUF_SIZE)
			post_write_message();
	}

	if (sync) {
		get_cpu_node()->get_thread_ctl().sleep();
		//thread_queue& tc = get_cpu_node()->get_thread_ctl();
		//tc.sleep_running_thread();
	}

	return r;
}

cause::type serial_ctrl::write_buf(iovec_iterator& iov_itr, uptr* bytes)
{
	buf_entry* buf = buf_queue.head();
	if (buf == 0) {
		buf = buf_append();
		if (buf == 0)
			return cause::NOMEM;
	}

	uptr total = 0;

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

	*bytes = total;

	if (sync) {
		thread_queue& tc = get_cpu_node()->get_thread_ctl();
		buf->set_bufsize(next_write);
		buf->set_client(tc.get_running_thread());
	}

	return cause::OK;
}

void serial_ctrl::on_write_message()
{
	transmit();
}

void serial_ctrl::on_write_message_(message* msg)
{
	serial_msg* _msg = static_cast<serial_msg*>(msg);
	serial_ctrl* serial = _msg->data;

	serial->write_posted = false;
	serial->on_write_message();
}

void serial_ctrl::post_write_message()
{
	if (write_posted) {
		return;
	}

	write_posted = true;

	cpu_node* cpu = get_cpu_node();
	cpu->post_soft_message(&write_msg);
}

void serial_ctrl::on_intr_message()
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

void serial_ctrl::on_intr_message_(message* msg)
{
	serial_msg* _msg = static_cast<serial_msg*>(msg);
	serial_ctrl* serial = _msg->data;

	serial->intr_posted = false;
	serial->on_intr_message();
}

void serial_ctrl::post_intr_message()
{
	intr_pending = true;

	// TODO: exclusive
	if (intr_posted)
		return;

	intr_posted = true;

	cpu_node* cpu = get_cpu_node();
	cpu->post_intr_message(&intr_msg);
	//cpu->get_thread_ctl().ready_message_thread_in_intr();
	cpu->switch_messenger_after_intr();
}

/// 割り込み発生時に呼ばれる。
void serial_ctrl::intr_handler(void* param)
{
	static_cast<serial_ctrl*>(param)->post_intr_message();
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
				thread_queue& tc =
				    get_cpu_node()->get_thread_ctl();
				tc.ready(client);
			}

			buf_mp->dealloc(buf_queue.remove_tail());
			buf = buf_queue.tail();
			buf_is_last = buf == buf_queue.head();
			next_read = 0;
		}
	}

	tx_fifo_queued = i;
}

}

namespace {
uptr tmp[(sizeof (serial_ctrl) + sizeof (uptr) - 1) / sizeof (uptr)];
}
file* create_serial()
{
	///////
	serial_ctrl::serial_ops.write = file::call_on_write<serial_ctrl>;
	///////

	//void* mem = memory::alloc(sizeof (serial_ctrl));
	void* mem = tmp;
	serial_ctrl* serial = new (mem) serial_ctrl(0x03f8, 4);

	serial->configure();

	return serial;
}

