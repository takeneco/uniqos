/// @file  page_table.hh
//
// (C) 2014 KATO Takeshi
//

#ifndef ARCH_X86_64_SOURCE_PAGE_TABLE_HH_
#define ARCH_X86_64_SOURCE_PAGE_TABLE_HH_

#include <pagetable.hh>


namespace x86 {

class page_table_traits;
typedef arch::page_table_tmpl<page_table_traits> page_table;

class page_table_traits
{
public:
	static cause::pair<uptr> acquire_page(page_table* x);
	static cause::t          release_page(page_table* x, u64 padr);

	static void* phys_to_virt(uptr adr) {
		return arch::map_phys_adr(adr, arch::page::PHYS_L1_SIZE);
	}
	static uptr virt_to_phys(void* adr) {
		return arch::unmap_phys_adr(adr, arch::page::PHYS_L1_SIZE);
	}
};

}  // namespace x86


#endif  // include guard

