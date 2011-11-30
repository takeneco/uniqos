/// @file  page_ctl.cc
/// @brief Physical page management.
//
// (C) 2010-2011 KATO Takeshi
//

#include "global_vars.hh"
#include "page_ctl.hh"
#include "pagetable.hh"


void page_ctl::detect_paging_features()
{
	struct regs {
		u32 eax, ebx, ecx, edx;
	} r[3];

	asm volatile ("cpuid" :
	    "=a"(r[0].eax), "=b"(r[0].ebx), "=c"(r[0].ecx), "=d"(r[0].edx) :
	    "a"(0x01));
	asm volatile ("cpuid" :
	    "=a"(r[1].eax), "=b"(r[1].ebx), "=c"(r[1].ecx), "=d"(r[1].edx) :
	    "a"(0x80000001));
	asm volatile ("cpuid" :
	    "=a"(r[2].eax), "=b"(r[2].ebx), "=c"(r[2].ecx), "=d"(r[2].edx) :
	    "a"(0x80000008));

	pse     = !!(r[0].edx & 0x00000008);
	pae     = !!(r[0].edx & 0x00000040);
	pge     = !!(r[0].edx & 0x00002000);
	pat     = !!(r[0].edx & 0x00010000);
	pse36   = !!(r[0].edx & 0x00020000);
	pcid    = !!(r[0].ecx & 0x00020000);

	nx      = !!(r[1].edx & 0x00100000);
	page1gb = !!(r[1].edx & 0x02000000);
	lm      = !!(r[1].edx & 0x20000000);

	padr_width = r[2].eax & 0x000000ff;
	vadr_width = (r[2].eax & 0x0000ff00) >> 8;
/*
	log()
	("pse=").u(u8(pse))
	(";pae=").u(u8(pae))
	(";pge=").u(u8(pge))
	(";pat=").u(u8(pat))
	(";pse36=").u(u8(pse36))
	(";pcid=").u(u8(pcid))
	(";nx=").u(u8(nx))
	(";page1gb=").u(u8(page1gb))
	(";lm=").u(u8(lm))
	(";padr_width=").u(u8(padr_width))
	(";vadr_width=").u(u8(vadr_width))();
*/
}

page_ctl::page_ctl()
{
	page_base[0].set_params(12, 0);
	page_base[1].set_params(18, &page_base[0]);
	page_base[2].set_params(21, &page_base[1]);
	page_base[3].set_params(27, &page_base[2]);
	page_base[4].set_params(30, &page_base[3]);
}

/// @brief 物理メモリの管理に必要なデータエリアのサイズを返す。
//
/// @param[in] _pmem_end 物理アドレスの終端アドレス。
/// @return ワークエリアのサイズをバイト数で返す。
uptr page_ctl::calc_workarea_size(uptr _pmem_end)
{
	return page_base[4].calc_buf_size(_pmem_end);
}

/// @param[in] _pmem_end 物理メモリの終端アドレス。
/// @param[in] buf  calc_workarea_size() が返したサイズのメモリへのポインタ。
/// @return true を返す。
bool page_ctl::init(uptr _pmem_end, void* buf)
{
	page_base[4].set_buf(buf, _pmem_end);

	detect_paging_features();

	return true;
}

bool page_ctl::load_free_range(u64 adr, u64 bytes)
{
	page_base[4].free_range(adr, adr + bytes - 1);

	return true;
}

void page_ctl::build()
{
	page_base[4].build_free_chain();
}

cause::stype page_ctl::alloc(arch::page::TYPE page_type, uptr* padr)
{
	return page_base[page_type].reserve_1page(padr);
}

cause::stype page_ctl::free(arch::page::TYPE page_type, uptr padr)
{
	return page_base[page_type].free_1page(padr);
}


namespace arch {
namespace page {

cause::stype alloc(TYPE page_type, uptr* padr)
{
	return global_vars::gv.page_ctl_obj->alloc(page_type, padr);
}

cause::stype free(TYPE page_type, uptr padr)
{
	return global_vars::gv.page_ctl_obj->free(page_type, padr);
}

}  // namespace page
}  // namespace arch

