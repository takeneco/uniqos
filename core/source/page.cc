/// @file   page.cc
/// @brief  Memory page alloc/free.
//
// (C) 2012-2013 KATO Takeshi
//

#include <page.hh>

#include <cpu_node.hh>


cause::t page_alloc(arch::page::TYPE page_type, uptr* padr)
{
	const cpu_id cpuid = arch::get_cpu_id();

	return page_alloc(cpuid, page_type, padr);
}

cause::t page_alloc(cpu_id cpuid, arch::page::TYPE page_type, uptr* padr)
{
	cpu_node* cpu = get_cpu_node(cpuid);

	return cpu->page_alloc(page_type, padr);
}

cause::t page_dealloc(arch::page::TYPE page_type, uptr padr)
{
	cpu_node* cpu = get_cpu_node();

	return cpu->page_dealloc(page_type, padr);
}

