/* FILE : include/fat.hpp
 * VER  : 0.0.1
 * LAST : 2008-01-02
 * (C)  : T.Kato 2009
 * DESC : FATファイルシステム
 */

#ifndef _INCLUDE_FAT_HPP_
#define _INCLUDE_FAT_HPP_

#include "types.hpp"

struct fat_header {
	__u8  jmp_opcode[3];
	__u8  maker[8];
	__u8  sector_size[2];
	__u8  secs_per_clu;
	__u8  reserved_secs[2];
	__u8  fats;
	__u8  root_ents[2];
	__u8  total_secs[2];
	__u8  media;
	__u8  secs_per_fat[2];
	__u8  secs_per_head[2];
	__u8  heads_per_cil[2];
	__u8  hidden_secs[4];
	__u8  bigtotal_secs[4];
	__u8  drive;
	__u8  unused;
	__u8  boot_sig;
	__u8  serial[4];
	__u8  volume[11];
	__u8  fstype[8];
};

struct dir_entry {
	__u8 name[11];
	__u8 attr;
	__u8 reversed1;
	__u8 ctime_cs;
	__u8 ctime[2];
	__u8 cdate[2];
	__u8 adate[2];
	__u8 reversed2[2];
	__u8 mtime[2];
	__u8 mdate[2];
	__u8 start[2];
	__u8 size[4];
};

#endif // _INCLUDE_FAT_HPP_
