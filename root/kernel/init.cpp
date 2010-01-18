/**
 * @file    kernel/init.cpp
 * @version 0.0.1.0
 * @date    2009-08-02
 * @author  Kato.T
 * @brief   カーネルの初期化。
 */
// (C) Kato.T 2009

#include "native.hpp"
#include "ktty.hpp"


inline static _u32 pci_config_addr(
	_u32 reg,
	_u32 func,
	_u32 dev,
	_u32 bus)
{
	return 0x80000000 |
		(reg    <<  0) |
		(func   <<  8) |
		(dev    << 11) |
		(bus    << 16);
}

void pcnet32_init(int bus, int dev, ktty* kt)
{
	_u32 pci_regs[0x10];

	for (int reg = 0; reg < 0x10; reg ++) {
		native_outl(pci_config_addr(reg * 4, 0, dev, bus), 0x0cf8);
		pci_regs[reg] = native_inl(0x0cfc);
		kt->putc(' ')->put32x(pci_regs[reg]);
	}

	_u16 ioaddr = (_u16)(pci_regs[4] & 0xfffc);

	kt->puts("\npcnet32 port = ")->put16x(ioaddr)->putc('\n');
	// resest
	native_inw(ioaddr + 0x14);

	native_outw(0, ioaddr + 0x12);
	int tmp = native_inw(ioaddr + 0x10) & 0xffff;
	native_outw(88, ioaddr + 0x12);
	if (tmp == 4 && native_inw(ioaddr + 0x12) == 88) {
		kt->puts("pcnet32 init ok\n");
	} else {
		kt->puts("pnnet32 init ng\n");
	}

	// version
	native_outw(88, ioaddr + 0x12);
	_u32 chver = native_inw(ioaddr + 0x10);
	native_outw(89, ioaddr + 0x12);
	chver |= native_inw(ioaddr + 0x10) << 16;
	kt->puts("pcnet32 chip ver = ")->put32x(chver)->putc('\n');
}

extern "C" void init()
{
	ktty* kt = create_ktty();

/*
	kt->put32x(0xdeadbeef)->putc('\n');

	native_outl(0x80000000, 0x0cf8);
	int pci0 = native_inl(0x0cfc);
	kt->put32x(pci0);
*/
	int pcnet32_dev = -1, pcnet32_bus = -1;

	for (int bus = 0; bus < 3; bus++) {
		for (int dev = 0; dev < 32; dev++) {
			int reg;
			int amd = 0;
			for (reg = 0; reg < 0x20; reg += 4) {
				_u32 ca = pci_config_addr(reg, 0, dev, bus);
				native_outl(ca, 0x0cf8);
				_u32 cd = native_inl(0x0cfc);
				if (reg == 0) {
					if (cd == 0xffffffff) {
						break;
					}
					else {
						kt
						->put8x(bus)
						->putc('-')
						->put8x(dev)
						->puts(": ");
					}
				}
				if (reg == 0 && (cd & 0x0000ffff) == 0x1022) {
					amd = 1;
				}
				if (amd == 1 && reg == 0x08 &&
				   (cd & 0xffffff00) == 0x02000000)
				{
					pcnet32_dev = dev;
					pcnet32_bus = bus;
				}
				kt->put32x(cd)->putc(' ');
			}
			if (reg > 0) {
				kt->putc('\n');
			}
		}
	}

	kt
	->puts("pcnet32 = ")
	->put8x(pcnet32_bus)
	->putc(':')
	->put8x(pcnet32_dev)
	->putc('\n');

	pcnet32_init(pcnet32_bus, pcnet32_dev, kt);

	extern int kern_head_addr, kern_tail_addr;
	kt
	->puts("kern_head_addr = ")->put32x((_u32)&kern_head_addr)->putc('\n')
	->puts("kern_tail_addr = ")->put32x((_u32)&kern_tail_addr)->putc('\n');
}
