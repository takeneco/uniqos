// @file   arch/x86_64/kernel/serialout.cc
// @author Kato Takeshi
// @brief  Output only serial port.
//
// (C) 2010 Kato Takeshi.

#include "output.hh"

#include "native.hh"
#include "kerninit.hh"


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

serial_output serial_out[2];

};

serial_output* serial_get_out(int i)
{
	return &serial_out[i];
}

void serial_output_init()
{
	new(&serial_out[0]) serial_output;
	new(&serial_out[1]) serial_output;

	serial_out[0].init(
	    serial_output::COM1_BASEPORT, serial_output::COM1_PICIRQ);
}

// @brief  Initialize.
void serial_output::init(u16 com_base_port, u16 com_pic_irq)
{
	base_port = com_base_port;
	//pic_irq = com_pic_irq;

	native_cli();

	intr_set_handler(0x20 + 4, serial_intr_com1_handler);
	intr_set_handler(0x20 + 3, serial_intr_com2_handler);

	// �̿����ԡ������곫��
	native_outb(0x80, base_port + LINE_CTRL);

	// �̿����ԡ��ɤλ��� 600[bps]
	native_outb(0xc0, base_port + BAUDRATE_LSB);
	native_outb(0x00, base_port + BAUDRATE_MSB);

	// �̿����ԡ������꽪λ(����������)
	native_outb(0x03, base_port + LINE_CTRL);

	// ����ԥ�����
	native_outb(0x0b, base_port + MODEM_CTRL);

	// 16550�ߴ��⡼�ɤ�����
	// FIFO��14bytes�ˤʤ롣
	native_outb(0xc9, base_port + FIFO_CTRL);

	// �����ߤ�ͭ����
	native_outb(0x03, base_port + INTR_ENABLE);
	// ̵����
	//native_outb(0x00, base_port + INT_ENABLE);

	out_buf_left = OUT_BUF_SIZE;

	native_sti();
}


int serial_output::write(
    const io_vector* vectors,
    int              vector_count,
    ucpu             offset)
{
	if (out_buf_left == 0) {
		u8 line_status = native_inb(base_port + LINE_STATUS);
		if ((line_status & 0x20) != 0)
			out_buf_left = OUT_BUF_SIZE;
	}

	while (out_buf_left == 0) {
	}

	char c = *reinterpret_cast<char*>(vectors->address);
	native_outb(c, base_port + TRANSMIT_DATA);
	out_buf_left -= 1;

	return cause::OK;
}

extern "C" void on_serial_intr_com1()
{
	u8 status = native_inb(serial_out[0].base_port + INTR_ID);
	if ((status & 7) == 0x1) {
		// Tx data/FIFO empty
		serial_out[0].out_buf_left = serial_output::OUT_BUF_SIZE;
	}
	kern_output* kout = kern_get_out();
	kout->put_str("com1intr status:")->put_u8hex(status)->put_c('\n');
}

extern "C" void on_serial_intr_com2()
{
	u8 status = native_inb(serial_out[1].base_port + INTR_ID);
	if ((status & 7) == 0x1) {
		// Tx data/FIFO empty
		serial_out[1].out_buf_left = serial_output::OUT_BUF_SIZE;
	}
	kern_output* kout = kern_get_out();
	kout->put_str("com2intr status:")->put_u8hex(status)->put_c('\n');
}
