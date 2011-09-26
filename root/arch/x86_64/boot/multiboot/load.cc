

#include "log.hh"
#include "multiboot2.h"
#include "vga.hh"

extern "C" void pre(u32* tag)
{
	text_vga tv;
	tv.init(80, 25, (void*)0xb8000);
	log_file lf(&tv);
	log_set(0, &lf);
	log_set(1, &lf);

	u32 size = *tag;
	tag += 2;

	log()("size : ").u(size)();

	u32 read = 8;
	while (read < size) {
		multiboot_tag* mbt = (multiboot_tag*)tag;
		switch (mbt->type) {
		case 1:
			log()("cmdline : [")((const char*)(tag + 2))("]")();
			break;
		case 2:
			log()("bootldr : [")((const char*)(tag + 2))("]")();
			break;
		case 3:
			log()("modules tag availavle.")();
			break;
		case 4:
			log()("basic memory : lower=").u(tag[2])
				("KB, upper=").u(tag[3])("KB")();
			break;
		case 5:
			log()("bios boot device : ").u(tag[2], 16)
				(", ").u(tag[3], 16)
				(", ").u(tag[4], 16)();
		}

		const u32 dec = (mbt->size + 7) & ~7;
		tag += dec/4;
		read += dec;
	}
}
