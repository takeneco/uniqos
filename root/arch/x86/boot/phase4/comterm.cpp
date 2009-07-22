/**
 * @file    arch/x86/boot/phase4/comterm.c
 * @version 0.0.1
 * @date    2009-07-06
 * @author  Kato.T
 *
 * 簡単なシリアルコンソール出力。
 */
// (C) Kato.T 2009

#include <cstddef>
#include "asmfunc.hpp"
#include "phase4.hpp"

enum PORT {
	RECEIVE_DATA  = 0,
	TRANSMIT_DATA = 0,
	BAUDRATE_LSB  = 0,
	BAUDRATE_MSB  = 1,
	INT_ENABLE    = 1,
	INT_ID        = 2,
	FIFO_CTRL     = 2,
	LINE_CTRL     = 3,
	MODEM_CTRL    = 4,
	LINE_STATUS   = 5,
	MODEM_STATUS  = 6,
};


/**
 * data_head か data_tail を data_buf 上で１つ進めたときの位置を返す。
 *
 * @param ptr data_head か data_tail。
 * @return ptr で指定したインデックスを１つ進めた位置。
 */
int com_term::next_data(int ptr)
{
	int next_ptr = ptr + 1;
	if (next_ptr >= sizeof data_buf) {
		next_ptr = 0;
	}

	return next_ptr;
}

/**
 * data_buf のデータを送信する。
 */
void com_term::tx_buf()
{
	while (out_buf_left > 0) {
		if (data_head == data_tail) {
			break;
		}
		native_outb(data_buf[data_head],
			base_port + TRANSMIT_DATA);
		data_head = next_data(data_head);
		out_buf_left--;
	}
}

/**
 * com_term クラスを初期化する。
 *
 * @param port ベースポート。
 */
void com_term::init(_u16 dev_port, _u16 pic_irq)
{
	base_port = dev_port;
	irq = pic_irq;
	data_head = data_tail = 0;

	// 通信スピードの設定開始
	native_outb(0x80, base_port + LINE_CTRL);

	// 通信スピード 600[bps]
	native_outb(0xc0, base_port + BAUDRATE_LSB);
	native_outb(0x00, base_port + BAUDRATE_MSB);

	// 通信スピード設定終了
	// 8ビット送受信開始
	native_outb(0x03, base_port + LINE_CTRL);

	// 制御ピン設定
	native_outb(0x0b, base_port + MODEM_CTRL);

	// FIFO コントロール
	native_outb(0xc9, base_port + FIFO_CTRL);

	// 割り込み設定
	native_outb(0x0f, base_port + INT_ENABLE);

	out_buf_size = 2;

	out_buf_left = out_buf_size;
}

/**
 * COM へ１文字出力する。
 *
 * @param ch 出力する文字。
 */
void com_term::putc(char ch)
{
	if (out_buf_left == 0) {
		for (int i = 0; i < 256; i++) {
			_u8 line_stat = native_inb(base_port + LINE_STATUS);
			if ((line_stat & 0x40) != 0) {
				out_buf_left = out_buf_size;
				break;
			}
		}
	}
	native_outb(ch, base_port + TRANSMIT_DATA);
	out_buf_left -= 1;
}

/**
 * 割り込みハンドラ。
 */
void com_term::on_interrupt()
{
	const _u8 iid = native_inb(base_port + INT_ID);

	if ((iid & 1) == 0) {
		// 割り込みなし
		return;
	}

	if ((iid & 0x0e) == 0x02) {
		// Tx FIFO empty

		out_buf_left = out_buf_size;
		tx_buf();
	}
}

