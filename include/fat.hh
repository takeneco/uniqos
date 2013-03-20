/* @file   include/fat.hh
 * @author Kato Takeshi
 * @brief  FATファイルシステムのヘッダ情報。
 *
 * (C) Kato Takeshi 2009
 */

#ifndef INCLUDE_FAT_HH_
#define INCLUDE_FAT_HH_

#include "basic_types.hh"


struct fat_header {
	u8  jmp_opcode[3];
	u8  maker[8];
	u8  sector_size[2];
	u8  secs_per_clu;
	u8  reserved_secs[2];
	u8  fats;
	u8  root_ents[2];
	u8  total_secs[2];
	u8  media;
	u8  secs_per_fat[2];
	u8  secs_per_head[2];
	u8  heads_per_cil[2];
	u8  hidden_secs[4];
	u8  bigtotal_secs[4];
	u8  drive;
	u8  unused;
	u8  boot_sig;
	u8  serial[4];
	u8  volume[11];
	u8  fstype[8];
};

struct dir_entry {
	u8 name[11];
	u8 attr;
	u8 reversed1;
	u8 ctime_cs;
	u8 ctime[2];
	u8 cdate[2];
	u8 adate[2];
	u8 reversed2[2];
	u8 mtime[2];
	u8 mdate[2];
	u8 start[2];
	u8 size[4];
};

#endif  // Include guard.
