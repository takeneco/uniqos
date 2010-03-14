/* @file   include/fat.hh
 * @author Kato Takeshi
 * @brief  FATファイルシステムのヘッダ情報。
 *
 * (C) Kato Takeshi 2009
 */

#ifndef _INCLUDE_FAT_HH_
#define _INCLUDE_FAT_HH_

#include "btypes.hh"

struct fat_header {
	_u8  jmp_opcode[3];
	_u8  maker[8];
	_u8  sector_size[2];
	_u8  secs_per_clu;
	_u8  reserved_secs[2];
	_u8  fats;
	_u8  root_ents[2];
	_u8  total_secs[2];
	_u8  media;
	_u8  secs_per_fat[2];
	_u8  secs_per_head[2];
	_u8  heads_per_cil[2];
	_u8  hidden_secs[4];
	_u8  bigtotal_secs[4];
	_u8  drive;
	_u8  unused;
	_u8  boot_sig;
	_u8  serial[4];
	_u8  volume[11];
	_u8  fstype[8];
};

struct dir_entry {
	_u8 name[11];
	_u8 attr;
	_u8 reversed1;
	_u8 ctime_cs;
	_u8 ctime[2];
	_u8 cdate[2];
	_u8 adate[2];
	_u8 reversed2[2];
	_u8 mtime[2];
	_u8 mdate[2];
	_u8 start[2];
	_u8 size[4];
};

#endif  // Include guard.
