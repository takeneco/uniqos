/// @file   core/elf.hh
/// @brief  ELF format data types.
//
// (C) 2013-2014 KATO Takeshi
//

#ifndef CORE_INCLUDE_CORE_ELF_HH_
#define CORE_INCLUDE_CORE_ELF_HH_

#include <core/basic.hh>


typedef u16 Elf64_Half;
typedef s16 Elf64_SHalf;
typedef s32 Elf64_Sword;
typedef u32 Elf64_Word;
typedef u64 Elf64_Xword;
typedef s64 Elf64_Sxword;
typedef u64 Elf64_Addr;
typedef u64 Elf64_Off;

typedef u16 Elf32_Half;
typedef s16 Elf32_SHalf;
typedef s32 Elf32_Sword;
typedef u32 Elf32_Word;
typedef u64 Elf32_Xword;
typedef s64 Elf32_Sxword;
typedef u32 Elf32_Addr;
typedef u32 Elf32_Off;

enum { EI_NIDENT     = 16 };

struct Elf64_Ehdr
{
	unsigned char e_ident[EI_NIDENT];  /// Magic number and elf info
	Elf64_Half    e_type;
	Elf64_Half    e_machine;
	Elf64_Word    e_version;
	Elf64_Addr    e_entry;             /// Entry point
	Elf64_Off     e_phoff;             /// Program header table file offset
	Elf64_Off     e_shoff;             /// Section header table file offset
	Elf64_Word    e_flags;
	Elf64_Half    e_ehsize;            /// ELF header size
	Elf64_Half    e_phentsize;         /// Program header table entry size
	Elf64_Half    e_phnum;             /// Program header table entry count
	Elf64_Half    e_shentsize;         /// Section header table entry size
	Elf64_Half    e_shnum;             /// Section header table entry count
	Elf64_Half    e_shstrndx;
};

enum {
	// e_ident
	EI_MAG0       = 0,     /// index of Elf_Ehdr::e_ident[]
	ELFMAG0       = 0x7f,  /// magic number value

	EI_MAG1       = 1,
	ELFMAG1       = 'E',

	EI_MAG2       = 2,
	ELFMAG2       = 'L',

	EI_MAG3       = 3,
	ELFMAG3       = 'F',

	EI_CLASS      = 4,
	ELFCLASSNONE  = 0,
	ELFCLASS32    = 1,
	ELFCLASS64    = 2,

	EI_DATA       = 5,
	ELFDATANONE   = 0,
	ELFDATA2LSB   = 1,
	ELFDATA2MSB   = 2,

	EI_VERSION    = 6,
	EV_NONE       = 0,
	// e_version
	EV_CURRENT    = 1,

	EI_OSABI      = 7,
	ELFOSABI_NONE = 0,

	EI_ABIVERSION = 8,

	EI_PAD        = 9,

	// e_type
	ET_NONE       = 0,
	ET_REL        = 1,
	ET_EXEC       = 2,
	ET_DYN        = 3,
	ET_CORE       = 4,
	ET_LOOS       = 0xfe00,
	ET_HIOS       = 0xfeff,
	ET_LOPROC     = 0xff00,
	ET_HIPROC     = 0xffff,

	// e_machine
	EM_X86_64     = 62,
};


struct Elf64_Phdr
{
	Elf64_Word  p_type;
	Elf64_Word  p_flags;
	Elf64_Off   p_offset;  /// Segment file offset
	Elf64_Addr  p_vaddr;
	Elf64_Addr  p_paddr;
	Elf64_Xword p_filesz;  /// Segment size in file
	Elf64_Xword p_memsz;   /// Segment size in memory
	Elf64_Xword p_align;   /// Segment alignment
};

enum {
	// p_type
	PT_NULL     = 0,
	PT_LOAD     = 1,
	PT_DYNAMIC  = 2,
	PT_INTERP   = 3,
	PT_NOTE     = 4,
	PT_SHLIB    = 5,
	PT_PHDR     = 6,
	PT_TLS      = 7,

	// p_flags
	PF_X        = 1 << 0,
	PF_W        = 1 << 1,
	PF_R        = 1 << 2,

};


struct Elf64_Shdr {
	Elf64_Word  sh_name;       /// Section name, index in string tbl
	Elf64_Word  sh_type;
	Elf64_Xword sh_flags;      /// Miscellaneous section attributes
	Elf64_Addr  sh_addr;       /// Section virtual addr at execution
	Elf64_Off   sh_offset;     /// Section file offset
	Elf64_Xword sh_size;       /// Size of section in bytes
	Elf64_Word  sh_link;       /// Index of another section
	Elf64_Word  sh_info;       /// Additional section information
	Elf64_Xword sh_addralign;  /// Section alignment
	Elf64_Xword sh_entsize;    /// Entry size if section holds table
};

enum {
	// sh_type
	SHT_NULL      = 0,
	SHT_PROGBITS  = 1,
	SHT_SYMTAB    = 2,
	SHT_STRTAB    = 3,
	SHT_RELA      = 4,
	SHT_HASH      = 5,
	SHT_DYNAMIC   = 6,
	SHT_NOTE      = 7,
	SHT_NOBITS    = 8,
	SHT_REL       = 9,
	SHT_SHLIB     = 10,
	SHT_DYNSYM    = 11,
	SHT_NUM       = 12,
	SHT_LOPROC    = 0x70000000,
	SHT_HIPROC    = 0x7fffffff,
	SHT_LOUSER    = 0x80000000,
	SHT_HIUSER    = 0xffffffff,

	// sh_flags
	SHF_WRITE     = 0x1,
	SHF_ALLOC     = 0x2,
	SHF_EXECINSTR = 0x4,
	SHF_MASKPROC  = 0xf0000000,
};


#endif  // Include guard

