/* FILE : cmdparse.h
 * DATE : 2008-11-17
 * VER  : 0.0.1
 * COPY : Copyright (C) T.Kato
 * DESC : コマンドラインオプション解析ツールのヘッダファイル
 */

#ifndef _CMDPARSE_H
#define _CMDPARSE_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

enum opt_style_param {
	OPTSTYLE_NULL,
	// このオプションが記述されると opt_parsed.yesno が 1 になる
	OPTSTYLE_YES,
	// OPTSTYLE_YES と同じだが、暗黙に'-'が付くショートオプション
	OPTSTYLE_SHORT_YES,
	// このオプションを記述されると opt_parsed.yesno が 0 になる
	OPTSTYLE_NO,
	// OPTSTYLE_NO と同じだが、暗黙に'-'が付くショートオプション
	OPTSTYLE_SHORT_NO,
	// このオプションが記述されると続く文字列が opt_parsed.ptrに格納される
	OPTSTYLE_AFTER,
	// このオプションが記述されると次の要素が opt_parsed.ptr に格納される
	OPTSTYLE_NEXT
};

/* オプションの指定方法
 */
struct opt_style {
	enum opt_style_param style;
	/* オプション指定文字列 */
	const char *match;
};

/* 同じ意味を持つオプションのセット
 */
struct opt_type {
	//int styles;
	const struct opt_style *style;
};

#define OPT_STYLE_NULL { OPTSTYLE_NULL, NULL }
#define OPT_TYPE_NULL { NULL }

struct opt_parsed {
	union {
		int yesno;
		// argv の中を指すポインタ
		const char *ptr;
	};
};

int option_parse(
	int args,
	const char *argv[],
	const struct opt_type *type_array,
	struct opt_parsed *parsed);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _CMDPARSE_H
