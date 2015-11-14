/// @file  serial.cc
/// @brief serial port.

//  Uniqos  --  Unique Operating System
//  (C) 2011-2015 KATO Takeshi
//
//  Uniqos is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  any later version.
//
//  Uniqos is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <arch/native_io.hh>
#include <irq_ctl.hh>
#include <core/cpu_node.hh>
#include <core/global_vars.hh>
#include <core/intr_ctl.hh>
#include <core/io_node.hh>
#include <core/mempool.hh>
#include <core/message.hh>
#include <core/new_ops.hh>
#include <core/spinlock.hh>


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

		chain_node<buf_entry> chain_node;
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
	chain_node<buf_entry>& chain_hook() { return f.chain_node; }

	void write(u32 i, u8 c) { buf()[i] = c; }
	u8 read(u32 i) { return buf()[i]; }

	void set_client(thread* t) { f.client_thread = t; }
	thread* get_client() { return f.client_thread; }

	void set_bufsize(uptr bs) { f.buf_size = bs; }
	uptr get_bufsize() const { return f.buf_size; }
};


/// @par 割込みの流れ
///  - 割込みが発生すると on_intr() が呼ばれる
///  - on_intr() は intr_msg を登録する。
///  - intr_msg から on_intr_msg() が呼ばれる。
///  - on_intr_msg() が割込みを判別する。
class serial_ctl : public io_node
{
	DISALLOW_COPY_AND_ASSIGN(serial_ctl);

	friend class io_node;

	const u16 base_port;
	const u16 irq_num;

	fchain<buf_entry, &buf_entry::chain_hook> buf_queue;

	/// バッファに書くときは buf_queue->head() の next_write に書く。
	u32 next_write;

	/// バッファを読むときは buf_queue->tail() の next_read から読む。
	u32 next_read;

	mempool* buf_mp;

	bool output_fifo_empty;
	int tx_fifo_queued;

	typedef intr_handler_with<serial_ctl*> serial_intr_hdr;
	serial_intr_hdr intr_hdr;

	typedef message_with<serial_ctl*> serial_msg;
	serial_msg write_msg;
	serial_msg intr_msg;

	spin_lock write_queue_lock;
	spin_lock write_msg_lock;
	spin_lock intr_msg_lock;

	bool write_posted;
	volatile bool intr_posted;
	volatile bool intr_pending;

public:
	/// @todo: do not use global var.
	static io_node::interfaces serial_ifs;

public:
	serial_ctl(u16 _base_port, u16 _irq_num);
	cause::t configure();

private:
	bool buf_is_empty() const {
		const buf_entry* h = buf_queue.front();
		return (h == 0) ||
		       (next_write == next_read && buf_queue.next(h) == 0);
	}
	buf_entry* buf_append() {
		buf_entry* buf = new (buf_mp->alloc()) buf_entry;
		if (buf == 0)
			return 0;
		write_queue_lock.lock();
		buf_queue.push_front(buf);
		write_queue_lock.unlock();
		next_write = 0;
		return buf;
	}
	buf_entry* get_next_buf();
	bool is_txfifo_empty() const;
	cause::pair<uptr> on_Write(offset off, const void* data, uptr bytes);
	cause::t on_io_node_write(offset* off, int iov_cnt, const iovec* iov);
	cause::pair<uptr> write_buf(offset off, const void* data, uptr bytes);
	cause::t write_buf(offset* off, iovec_iterator& iov_itr);

	void post_write_msg();
	void on_write_msg();
	static void _on_write_msg(message* msg);

	void post_intr_msg();
	void on_intr_msg();
	static void _on_intr_msg(message* msg);
	static void on_intr(intr_handler* h);

	void transmit();

public:
	void dump();
};

//TODO:
io_node::interfaces serial_ctl::serial_ifs;

serial_ctl::serial_ctl(u16 _base_port, u16 _irq_num) :
	base_port(_base_port),
	irq_num(_irq_num),
	next_read(0),
	output_fifo_empty(true),
	tx_fifo_queued(0),
	intr_pending(false)
{
	ifs = &serial_ifs;
}

cause::t serial_ctl::configure()
{
	u32 vec = 0xffffffff;
	arch::irq_interrupt_map(4, &vec);

	intr_hdr.data = this;
	intr_hdr.handler = on_intr;
	global_vars::core.intr_ctl_obj->install_handler(vec, &intr_hdr);

	write_msg.handler = _on_write_msg;
	write_msg.data = this;
	write_posted = false;

	intr_msg.handler = _on_intr_msg;
	intr_msg.data = this;
	intr_posted = false;

	// 通信スピード設定開始
	arch::ioport_out8(0x80, base_port + LINE_CTRL);

	// 通信スピードの指定 600[bps]
	arch::ioport_out8(0xc0, base_port + BAUDRATE_LSB);
	arch::ioport_out8(0x00, base_port + BAUDRATE_MSB);

	// fastest
	arch::ioport_out8(0x01, base_port + BAUDRATE_LSB);
	arch::ioport_out8(0x00, base_port + BAUDRATE_MSB);

	// 通信スピード設定終了(送受信開始)
	arch::ioport_out8(0x03, base_port + LINE_CTRL);

	// 制御ピン設定
	arch::ioport_out8(0x0b, base_port + MODEM_CTRL);

	// 16550互換モードに設定
	// FIFOが14bytesになる。
	// FIFOをクリアする。
	arch::ioport_out8(0xcf, base_port + FIFO_CTRL);

	// 割り込みを有効化
	arch::ioport_out8(0x03, base_port + INTR_ENABLE);
	// 無効化
	//native::outb(0x00, base_port + INTR_ENABLE);

	auto mp = mempool::acquire_shared(buf_entry::SIZE);
	if (is_fail(mp))
		return mp.cause();

	buf_mp = mp;

	return cause::OK;
}

buf_entry* serial_ctl::get_next_buf()
{
	buf_entry* buf = buf_queue.front();

	if (buf == 0 || next_write >= buf->get_bufsize())
		buf = buf_append();

	return buf;
}

bool serial_ctl::is_txfifo_empty() const
{
	const u8 line_status = arch::ioport_in8(base_port + LINE_STATUS);

	return (line_status & 0x20) != 0;
}

cause::pair<uptr> serial_ctl::on_Write(
    offset off, const void* data, uptr bytes)
{
	cause::pair<uptr> r;
	{
		preempt_disable_section _pds;

		r = write_buf(off, data, bytes);

		if (tx_fifo_queued < DEVICE_TXBUF_SIZE)
			post_write_msg();
	}

	if (false /* sync */ && r.data() != 0) {
		sleep_current_thread();
	}

	return r;
}

/// @brief  Write to buffer.
/// @param[out] bytes  write bytes.
cause::t serial_ctl::on_io_node_write(
    offset* off, int iov_cnt, const iovec* iov)
{
	const offset before_off = *off;
	iovec_iterator iov_itr(iov, iov_cnt);

	cause::t r;
	{
		preempt_disable_section _pds;

		r = write_buf(off, iov_itr);

		if (tx_fifo_queued < DEVICE_TXBUF_SIZE)
			post_write_msg();
			//transmit();
	}

	if (false /* sync */ && *off != before_off) {
		sleep_current_thread();
	}

	return r;
}

cause::pair<uptr> serial_ctl::write_buf(
    offset /*off*/, const void* data, uptr bytes)
{
	buf_entry* buf = 0;
	cause::t r = cause::OK;
	const u8* _data = static_cast<const u8*>(data);

	uptr write_bytes = 0;

	for (uptr i = 0; i < bytes; ++i) {
		u8 c = *_data++;

		if (c == '\n') {
			buf = get_next_buf();
			if (!buf) {
				r = cause::NOMEM;
				break;
			}
			buf->write(next_write++, '\r');
		}

		buf = get_next_buf();
		if (!buf) {
			r = cause::NOMEM;
			break;
		}

		buf->write(next_write++, c);
		++write_bytes;
	}

	if (false /* sync */ && buf) {
		buf->set_bufsize(next_write);
		buf->set_client(get_current_thread());
	}

	return make_pair(r, write_bytes);
}

cause::t serial_ctl::write_buf(offset* off, iovec_iterator& iov_itr)
{
	buf_entry* buf = 0;
	uptr total = 0;
	cause::t r = cause::OK;

	for (;;) {
		const u8* c = iov_itr.next_u8();
		if (!c)
			break;

		if (*c == '\n') {
			buf = get_next_buf();
			if (!buf) {
				r = cause::NOMEM;
				break;
			}
			buf->write(next_write++, '\r');
		}

		buf = get_next_buf();
		if (!buf) {
			r = cause::NOMEM;
			break;
		}

		buf->write(next_write++, *c);
		++total;
	}

	*off += total;

	if (false /* sync */ && buf) {
		buf->set_bufsize(next_write);
		buf->set_client(get_current_thread());
	}

	return r;
}

void serial_ctl::post_write_msg()
{
	{
		spin_lock_section_np _wml_sec(write_msg_lock);

		if (write_posted)
			return;

		write_posted = true;
	}

	post_message(&write_msg);
}

void serial_ctl::on_write_msg()
{
	write_msg_lock.lock_np();

	write_posted = false;

	write_msg_lock.unlock_np();

	transmit();
}

void serial_ctl::_on_write_msg(message* msg)
{
	serial_msg* _msg = static_cast<serial_msg*>(msg);
	serial_ctl* serial = _msg->data;

	serial->on_write_msg();
}

void serial_ctl::post_intr_msg()
{
	intr_pending = true;

	{
		spin_lock_section_np _sls_iwl(intr_msg_lock);

		// TODO: exclusive
		if (intr_posted)
			return;

		intr_posted = true;
	}

	arch::post_intr_message(&intr_msg);
}

void serial_ctl::on_intr_msg()
{
	intr_msg_lock.lock();
	intr_posted = false;
	intr_msg_lock.unlock();

	intr_pending = false;

	const u8 intr_id = arch::ioport_in8(base_port + INTR_ID);

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

void serial_ctl::_on_intr_msg(message* msg)
{
	serial_msg* _msg = static_cast<serial_msg*>(msg);
	serial_ctl* serial = _msg->data;

	serial->on_intr_msg();
}

/// 割り込み発生時に呼ばれる。
void serial_ctl::on_intr(intr_handler* h)
{
	static_cast<serial_intr_hdr*>(h)->data->post_intr_msg();
}

/// デバイスのFIFOのサイズだけ送信する。
/// デバイスのFIFOが空になったら呼ばれる。
//
/// TODO: exclusive
void serial_ctl::transmit()
{
	if (buf_is_empty()) {
		output_fifo_empty = true;
		return;
	}

	buf_entry* buf = buf_queue.back();
	bool buf_is_last = buf == buf_queue.front();

	int i;
	for (i = tx_fifo_queued; i < DEVICE_TXBUF_SIZE; ++i) {
		if (intr_pending)
			break;

		if (buf == 0)
			break;
		if (buf_is_last && next_write == next_read)
			break;

		arch::ioport_out8(buf->read(next_read++), base_port + TRANSMIT_DATA);

		if (next_read == buf->get_bufsize()) {

			thread* client = buf->get_client();
			if (client) {
				client->ready();
			}

			write_queue_lock.lock();
			buf_entry* tmp = buf_queue.pop_back();
			write_queue_lock.unlock();
			buf_mp->dealloc(tmp);
			buf = buf_queue.back();
			buf_is_last = buf == buf_queue.front();
			next_read = 0;
		}
	}

	tx_fifo_queued = i;
}

}

namespace {
uptr tmp[(sizeof (serial_ctl) + sizeof (uptr) - 1) / sizeof (uptr)];
}
io_node* create_serial()
{
	///////
	serial_ctl::serial_ifs.init();

	serial_ctl::serial_ifs.Write = io_node::call_on_Write<serial_ctl>;
	serial_ctl::serial_ifs.write = io_node::call_on_io_node_write<serial_ctl>;
	///////

	void* mem = tmp;
	serial_ctl* serial = new (mem) serial_ctl(0x03f8, 4);

	serial->configure();

	return serial;
}

