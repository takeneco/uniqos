/* FILE : tools/mkfdimg.cpp
 * VER  : 0.0.4
 * LAST : 2009-05-17
 * (C) 2008-2009 Kato.T
 * 
 * mkfdimgコマンド。FAT12フォーマットのFDイメージを作る。
 */

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "cmdparse.h"
#include "types.hpp"
#include "fat.hpp"

const off_t IMG_LENGTH = 1024 * 1440;

struct fat_params {
	int reserved;
};

result::stype write_header(int imgfd, const struct fat_params* params)
{
	fat_header h;

	// 0x00: jmp 0x3c; nop; (3 bytes)
	h.jmp_opcode[0] = 0xeb;
	h.jmp_opcode[1] = 0x3c;
	h.jmp_opcode[2] = 0x90;

	// 0x03: maker name (8 bytes)
	h.maker[0] = 'R';
	h.maker[1] = 'O';
	h.maker[2] = 'O';
	h.maker[3] = 'T';
	h.maker[4] = 'I';
	h.maker[5] = 'N';
	h.maker[6] = 'S';
	h.maker[7] = 'T';

	// 0x0b: sector size (2 bytes)
	cpu_to_le16(512, &h.sector_size[0], &h.sector_size[1]);

	// 0x0d: sectors / cluster (1 byte)
	h.secs_per_clu = 1;

	// 0x0e: reversed sectors from 0 (2 bytes)
	cpu_to_le16(params->reserved, &h.reserved_secs[0], &h.reserved_secs[1]);

	// 0x10: number of FATs (1 byte)
	h.fats = 2;

	// 0x11: root entries (2 bytes)
	cpu_to_le16(224, &h.root_ents[0], &h.root_ents[1]);

	// 0x13: total sectors (if 0 then FAT32) (2 bytes)
	cpu_to_le16(2880, &h.total_secs[0], &h.total_secs[1]);

	// 0x15: media descriptor 0xf0:floppy (1 byte)
	h.media = 0xf0;

	// 0x16: sectors / FAT (2 bytes)
	cpu_to_le16(9, &h.secs_per_fat[0], &h.secs_per_fat[1]);

	// 0x18: sectors / track (2 bytes)
	cpu_to_le16(18, &h.secs_per_head[0], &h.secs_per_head[1]);

	// 0x1a: number of disk heads (2 bytes)
	cpu_to_le16(2, &h.heads_per_cil[0], &h.heads_per_cil[1]);

	// 0x1c: hidden sectors (4 bytes)
	cpu_to_le32(0, &h.hidden_secs[0], &h.hidden_secs[1],
		&h.hidden_secs[2], &h.hidden_secs[3]);

	// 0x20: total sectors (FAT32) (4 bytes)
	cpu_to_le32(0, &h.bigtotal_secs[0], &h.bigtotal_secs[1],
		&h.bigtotal_secs[2], &h.bigtotal_secs[3]);

	// 0x24: drive number (1 byte)
	h.drive = 0;

	// 0x25: unused (1 byte)
	h.unused = 0;

	// 0x26: boot signature (1 byte)
	h.boot_sig = 0x29;

	// 0x27: serial number (4 bytes)
	cpu_to_le32(0, &h.serial[0], &h.serial[1], &h.serial[2], &h.serial[3]);

	// 0x2b: volume label (11 bytes)
	memcpy(&h.volume, "ROOTOS-BOOT", 11);

	// 0x36: fs type (8 bytes)
	memcpy(&h.fstype, "FAT12   ", 8);

	lseek(imgfd, 0, SEEK_SET);

	if (write(imgfd, &h, sizeof h) != sizeof h)
		return result::FAIL;

	return result::OK;
}

/**
 * ブートセクタのシグネチャを書く
 * @param imgfd 書き込み先イメージのディスクリプタ
 * @retval result::OK 成功した。
 * @retval result::FAIL 失敗した。
 */
result::stype write_sig(int imgfd)
{
	__u8 sig[2] = { 0x55, 0xaa };

	lseek(imgfd, 510, SEEK_SET);

	if (write(imgfd, sig, sizeof sig) != sizeof sig)
		return result::FAIL;

	return result::OK;
}

/* FAT領域を書く
 * imgfd  : 書き込み先イメージのディスクリプタ
 * params : FATのパラメータ
 * 戻り値 :
 * 	成功したときは result::OK を返す
 *	失敗したときは result::FAIL を返す
 */
result::stype write_fats(int imgfd, fat_params* params)
{
	_u8 fat[512 * 9] = { 0xf0, 0xff, 0xff, 0 };
	off_t ptr;

	ptr = params->reserved * 512;
	lseek(imgfd, ptr, SEEK_SET);
	if (write(imgfd, fat, sizeof fat) != sizeof fat)
		return result::FAIL;

	ptr = (params->reserved + 9) * 512;
	lseek(imgfd, ptr, SEEK_SET);
	if (write(imgfd, fat, sizeof fat) != sizeof fat)
		return result::FAIL;

	return result::OK;
}

static const struct opt_style optstyle_reversed[] = {
	{ OPTSTYLE_NEXT, "-r" },
	OPT_STYLE_NULL
};
static const struct opt_type cmd_option[] = {
	{ optstyle_reversed },
	OPT_TYPE_NULL
};

int main(int argc, const char *argv[])
{
	struct opt_parsed parsed[1] = { { { 0 } } };
	const char *path = 0;
	struct fat_params params;

	int imgfd;
	result::type result;

	argc--;
	argv++;

	params.reserved = 1;

	while (argc > 0) {
		int arginc = option_parse(argc, argv, cmd_option, parsed);
		argc -= arginc;
		argv += arginc;
		if (argc > 0) {
			if (path) {
				fprintf(stderr, "Same paths presented.\n");
			}
			else {
				argc--;
				path = *argv++;
			}
		}
	}

	if (parsed[0].ptr != NULL) {
		params.reserved = strtol(parsed[0].ptr, NULL, 0);
	}

	imgfd = creat(path, 0777);
	if (imgfd == -1) {
		abort();
	}

	ftruncate(imgfd, IMG_LENGTH);

	result = write_header(imgfd, &params);
	if (result::isfail(result)) {
		fprintf(stderr, "write_header failed.\n");
		return 1;
	}

	result = write_sig(imgfd);
	if (result::isfail(result)) {
		fprintf(stderr, "write_sig failed.\n");
		return 1;
	}

	result = write_fats(imgfd, &params);
	if (result::isfail(result)) {
		fprintf(stderr, "write_fat failed.\n");
		return 1;
	}

	close(imgfd);

	return 0;
}
