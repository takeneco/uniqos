/// @file   misc.hh
//
// (C) 2011 KATO Takeshi
//


struct acpi_memmap {
	enum type_value {
		MEMORY = 1,
		RESERVED = 2,
		ACPI = 3,
		NVS = 4,
		UNUSUABLE = 5
	};
	u64 base;
	u64 length;
	u32 type;
	u32 attr;
};

struct mem_enum {
	bool avoid;
	const void* entry;
};
void  mem_init();
void  mem_add(uptr adr, uptr bytes, bool avoid);
void* mem_alloc(uptr bytes, uptr align, bool forget);
void  mem_free(void* p);
bool  mem_reserve(uptr adr, uptr bytes, bool forget);
void  mem_alloc_info(mem_enum* me);
bool  mem_alloc_info_next(mem_enum* me, uptr* adr, uptr* bytes);

cause::stype pre_load(u32 magic, u32* tag);
cause::stype post_load(u32* tag);

class text_vga;
extern text_vga vga_dev;

class log_file;
extern log_file vga_log;

