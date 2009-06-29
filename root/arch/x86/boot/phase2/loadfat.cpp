/* FILE : arch/x86/boot/phase2/loadfat.cpp
 * VER  : 0.0.4
 * LAST : 2009-05-19
 * (C) Kato.T 2009
 *
 * CHS 経由のディスクアクセスで FAT からファイルを読み込む
 * FAT12専用。
 */

#include "loadfat.hpp"

asm (".code16gcc");

/*
 * disk_info クラスの定義
 */

/**
 * INT 13h からセクタ単位でディスクを読み込む。
 * @param block ブロック番号(中でCHSにマップする)。
 * @param count 読み込むセクタ数。
 * @param dsegm 読み込み先セグメント。
 * @param daddr 読み込み先アドレス。
 * @return 成功したときは 0 を返す。
 * @return 失敗したときはエラーコードを返す。
 *
 * 読み込み先は dsegm:daddr になる。
 */
int disk_info::read_secs(
	_u16 start, _u16 count, _u16 dsegm, _u16 daddr) const
{
	_u8  sect = start % secs_per_head;
	_u16 track = start / secs_per_head;
	_u16 to = daddr;

	asm volatile ("movw %%ax, %%es" : : "a" (dsegm));

	while (count > 0) {
		_u16 head = track % heads_per_cil;
		_u16 cilin = track / heads_per_cil;

		_u8 num = count;
		if (num > (secs_per_head - sect))
			num = secs_per_head - sect;

		count -= num;

		_u16 a;
		// 失敗しても８回繰り返す
		for (int i = 0; i < 8; i++) {
			a = 0x0200 | num;
			const _u16 c = (cilin & 0x00ff) << 8
				| (cilin & 0x0300) >> 2
				| (sect + 1);
			const _u16 d = head << 8 | dev;

			asm volatile ("int $0x13" :
				"=a" (a) :
				"a" (a), "b" (to), "c" (c), "d" (d));
			if ((a & 0xff00) == 0)
				break;
		}
		if (a & 0xff00) {
			return a >> 8;
		}

		to += bytes_per_sec * num;
		track++;
		sect = 0;
	}

	return 0;
}

/*
 * fat_info クラスの定義
 */

/**
 * メモリに展開済みのFATヘッダからパラメータを読み込む
 * @param dev ドライブ番号
 * @param raw FATヘッダへのポインタ
 */
fat_info::fat_info(int dev, _u8* raw)
	: head(raw)
{
	const fat_header* p = reinterpret_cast<fat_header*>(raw);

	disk_info::set_params(dev,
		le16_to_cpu(p->sector_size[0], p->sector_size[1]),
		le16_to_cpu(p->secs_per_head[0], p->secs_per_head[1]),
		le16_to_cpu(p->heads_per_cil[0], p->heads_per_cil[1]));

	secs_per_clu = p->secs_per_clu;

	reserved_secs = le16_to_cpu(p->reserved_secs[0], p->reserved_secs[1]);

	fats = p->fats;

	rootents_num = le16_to_cpu(p->root_ents[0], p->root_ents[1]);

	secs_per_fat = le16_to_cpu(p->secs_per_fat[0], p->secs_per_fat[1]);

	rootents_offset = bytes_per_sec * (reserved_secs + secs_per_fat * fats);

	// データブロックの開始位置
	data_block = (rootents_offset + sizeof (dir_entry) * rootents_num);
	data_block = (data_block + bytes_per_sec - 1) / bytes_per_sec;
}

/**
 * クラスタの後続クラスタ番号を返す。
 * @param prev クラスタ番号
 * @return prev の次のクラスタ番号を返す。
 * @return prev が最終クラスタの場合は 0x0ff8 から 0x0fff の間の値を返す。
 *
 */
int fat_info::next_clu(int prev) const
{
	const _u8* fat = head
		+ bytes_per_sec * reserved_secs;
	const int p1 = (prev >> 1) * 3;
	const int p2 = prev & 1;

	int next = p2 == 0 ?
		fat[p1 + 0] | (fat[p1 + 1] & 0x0f) << 8 :
		fat[p1 + 2] << 4 | (fat[p1 + 1] & 0xf0) >> 4;

	return next;
}

/// ルートディレクトリエントリからファイルを探す
/**
 * 引数
 * - name \n
 *   探すファイル名。文字数が少ない場合は' 'で埋める。
 * 戻り値
 * - 発見したファイルのディレクトリエントリへのポインタを返す。
 * - ファイルがない場合は 0 を返す。
 */
dir_entry* fat_info::find_from_dir(const char name[8+3])
{
	dir_entry* de = rootents();

	int n = rootents_num;
	for (int i = 0; i < n; i++) {
		dir_entry* p = de + i;
		if (p->name[0] == name[0]
		 && p->name[1] == name[1]
		 && p->name[2] == name[2]
		 && p->name[3] == name[3]
		 && p->name[4] == name[4]
		 && p->name[5] == name[5]
		 && p->name[6] == name[6]
		 && p->name[7] == name[7]
		 && p->name[8] == name[8]
		 && p->name[9] == name[9]
		 && p->name[10] == name[10]) {
			return p;
		}
	}

	return 0;
}

/**
 * ファイルをセグメントサイズ(64KiB)だけ読み込む
 * @param src_clu[IN,OUT]
 *        ファイルの先頭クラスタ番号。
 *        読み込んだデータに続きがある場合は、後続クラスタ番号が格納される。
 *        src_clu = 0xffff のときは、ファイル末尾までの読み込みが完了している。
 *        NULL は入力禁止。
 * @param dest_segm
 *        ファイルの読み込み先セグメントアドレス。
 *        dest_segm:0 から書き込む。
 * @retval result::OK
 *         読み込みが完了した。
 */
result::type fat_info::read_to_segment(int* src_clu, int dest_segm) const
{
	if (*src_clu >= 0x0ff8) {
		*src_clu = 0xffff;
		return result::OK;
	}

	int clu = *src_clu;
	int daddr = 0;
	const int clusize = bytes_per_sec * secs_per_clu;
	// １セグメントに含まれるクラスタ数。
	// １セグメントをクラスタサイズで割りきれる前提が必要。
	const int n = 0x10000 / clusize;

	for (int i = 0; i < n; i++) {
		// クラスタ番号は２から始まるため(clu - 2)とする。
		int block_start = data_block + (clu - 2) * secs_per_clu;

		int error = disk_info::read_secs(
			block_start, secs_per_clu, dest_segm, daddr);
		if (error != 0)
			return result::FAIL;

		clu = next_clu(clu);
		if (clu >= 0x0ff8)
			break;

		daddr += clusize;
	}

	if (clu >= 0x0ff8) {
		// ファイル末尾まで読み込み完了した。
		*src_clu = 0xffff;
	}
	else {
		// 後続のクラスタ番号
		*src_clu = clu;
	}

	return result::OK;
}
