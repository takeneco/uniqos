/// @file  serial.cc
/// @brief serial port.
//
// (C) 2011 KATO Takeshi
//

#include "fileif.hh"
#include "memory_allocate.hh"
#include "native_ops.hh"
#include "placement_new.hh"


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

class serial_ctrl : public io_interface
{
	const u16 base_port;
	const u16 irq_num;

public:
	static io_ops serial_ops;

public:
	serial_ctrl(u16 base_port_, u16 irq_num_);

	cause::stype configure();

	static int write(
	    io_interface*    self,
	    const io_vector* vecs,
	    int              vec_count,
	    u64              offset);
};

io_ops serial_ctrl::serial_ops;

serial_ctrl::serial_ctrl(u16 base_port_, u16 irq_num_) :
    base_port(base_port_),
    irq_num(irq_num_)
{
	ops = &serial_ops;
}

cause::stype serial_ctrl::configure()
{
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

	return cause::OK;
}

int serial_ctrl::write(
    io_interface*    self,
    const io_vector* vecs,
    int              vec_count,
    u64              offset)
{
	serial_ctrl* serial = reinterpret_cast<serial_ctrl*>(self);

	io_vector_iterator itr(vecs, vec_count);

	const u8* c = itr.next_u8();
	if (c == 0)
		return 0;
	native::outb(*c, serial->base_port + TRANSMIT_DATA);

	c = itr.next_u8();
	if (c == 0)
		return 0;
	native::outb(*c, serial->base_port + TRANSMIT_DATA);

	return 0;
};

}

io_interface* create_serial()
{
	///////
	serial_ctrl::serial_ops.write = serial_ctrl::write;
	///////

	void* mem = memory::alloc(sizeof (serial_ctrl));
	serial_ctrl* serial = new (mem) serial_ctrl(0x03f8, 4);

	serial->configure();

	return serial;
}

