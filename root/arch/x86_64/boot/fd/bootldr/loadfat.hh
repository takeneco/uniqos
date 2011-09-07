// @file   arch/x86_64/boot/bootldr/loadfat.hh
//
// (C) KATO Takeshi 2010
//

#ifndef ARCH_X86_BOOT_BOOTLDR_LOADFAT_HH_
#define ARCH_X86_BOOT_BOOTLDR_LOADFAT_HH_

#include "basic_types.hh"
#include "fat.hh"


asm (".code16gcc");

/*
 * disk_info クラスの宣言
 */

class disk_info
{
protected:
	int dev;
	int bytes_per_sec;
	int secs_per_head;
	int heads_per_cil;

public:
	void set_params(int d, int bps, int sph, int hpc);
	int read_secs(unsigned short start, unsigned short count,
		u16 dsegm, u16 daddr) const;
};

inline void disk_info::set_params(int d, int bps, int sph, int hpc) {
	dev = d;
	bytes_per_sec = bps;
	secs_per_head = sph;
	heads_per_cil = hpc;
}

/*
 * fat_info クラスの宣言
 */

class fat_info : public disk_info
{
	unsigned char* head;
	int secs_per_clu;
	int reserved_secs;
	int fats;
	int rootents_num;
	int secs_per_fat;

	int rootents_offset;
	int data_block;

	dir_entry* rootents();
public:
	fat_info(int dev, unsigned char* raw);

	dir_entry* find_from_dir(const char name[]);
	int next_clu(int prev) const;

	int rootent_endsec() const;
	int read_to_segment(int* src_clus, int dest_segm) const;
};

/// ブートセクタから RDE までのセクタ数を返す。
inline int fat_info::rootent_endsec() const
{
	return
		// ブートセクタとブートプログラムの領域
		reserved_secs
		// FAT領域
		+ secs_per_fat * fats
		// RDE領域
		+ sizeof (dir_entry) * rootents_num / bytes_per_sec;
}

/// メモリ上のルートディレクトリエントリへのポインタを返す。
inline dir_entry* fat_info::rootents() {
	return reinterpret_cast<dir_entry*>(head + rootents_offset);
}


#endif  // include guard

