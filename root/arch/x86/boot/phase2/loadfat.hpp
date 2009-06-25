/* FILE : arch/x86/boot/phase2/loadfat.hpp
 * VER  : 0.0.2
 * LAST : 2009-05-17
 * (C) T.Kato 2009
 */

#ifndef _ARCH_X86_BOOT_PHASE2_LOADFAT_HPP_
#define _ARCH_X86_BOOT_PHASE2_LOADFAT_HPP_

#include "btypes.hpp"
#include "fat.hpp"

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
	int read_secs(__u16 start, __u16 count,
		__u16 dsegm, __u16 daddr) const;
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
	__u8* head;
	int secs_per_clu;
	int reserved_secs;
	int fats;
	int rootents_num;
	int secs_per_fat;

	int rootents_offset;
	int data_block;

	dir_entry* rootents();
public:
	fat_info(int dev, __u8* raw);

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

#endif // _ARCH_X86_BOOT_PHASE2_LOADFAT_HPP_
