/* FILE : cmdparse.c
 * DATE : 2008/11/10
 * VER  : 0.0.1
 * COPY : Copyright (C) T.Kato
 * DESC : コマンドラインオプション解析ツール
 */

#include <string.h>
#include "cmdparse.h"

/* str1 と str2 の先頭から str1 の長さだけ比較する。
 * 同じならば、str1 の '\0' に相当する位置の str2 中のポインタを返す
 * 等しくなければ NULL を返す
 */
static const char * strpart(const char *str1, const char *str2)
{
	int i;

	if (!str1 || !str2)
		return NULL;

	for (i = 0; str1[i]; i++) {
		if (str1[i] != str2[i])
			return NULL;
	}

	return str2 + i;
}

/* コマンドラインの1つをショートオプションとして解析する
 */
static int shortargparse(
	const char *argv0,
	const struct opt_type *type_array,
	struct opt_parsed *parsed)
{
	int type;
	const char *arg;

	if (!argv0 || !type_array || !parsed)
		return 0;

	arg = argv0;
	while (*arg != '\0') {
		int matched = 0;
		for (type = 0; matched == 0 && type_array[type].style != NULL; type++) {
			int style;
			for (style = 0; type_array[type].style[style].style != OPTSTYLE_NULL; style++) {
				const int styleis = type_array[type].style[style].style;
				if (styleis == OPTSTYLE_SHORT_YES || styleis == OPTSTYLE_SHORT_NO)
				{
					const char *end;
					end = strpart(type_array[type].style[style].match, arg);
					if (end) {
						arg = end;
						matched = 1;
						break;
					}
				}
			}
		}
		if (matched == 0)
			return 0;
	}

	arg = argv0;
	while (*arg != '\0') {
		int matched = 0;
		for (type = 0; matched == 0 && type_array[type].style != NULL; type++) {
			int style;
			for (style = 0; type_array[type].style[style].style != OPTSTYLE_NULL; style++) {
				const int styleis = type_array[type].style[style].style;
				if (styleis == OPTSTYLE_SHORT_YES || styleis == OPTSTYLE_SHORT_NO)
				{
					const char *end;
					end = strpart(type_array[type].style[style].match, arg);
					if (end) {
						parsed[type].yesno = styleis == OPTSTYLE_SHORT_YES;
						arg = end;
						matched = 1;
						break;
					}
				}
			}
		}
		if (matched == 0)
			return 0;
	}

	return 1;
}

/* コマンドラインを1つだけ解析する
 */
static int argparse(
	int args,
	const char *argv[],
	const struct opt_type *type_array,
	struct opt_parsed *parsed)
{
	int arg = 0;
	int type;

	for (type = 0; arg == 0 && type_array[type].style != NULL; type++) {
		int style;
		for (style = 0; arg == 0 && type_array[type].style[style].style != OPTSTYLE_NULL; style++)
		{
			const int styleis = type_array[type].style[style].style;

			if ((styleis == OPTSTYLE_YES || styleis == OPTSTYLE_NO) &&
				0 == strcmp(type_array[type].style[style].match, argv[arg]))
			{
				parsed[type].yesno = styleis == OPTSTYLE_YES;
				arg++;
			}
			else if (styleis == OPTSTYLE_AFTER)
			{
				const char *end;
				end = strpart(type_array[type].style[style].match, argv[arg]);
				if (end != NULL) {
					parsed[type].ptr = end;
					arg++;
				}
			}
			else if (styleis == OPTSTYLE_NEXT &&
				0 == strcmp(type_array[type].style[style].match, argv[arg]))
			{
				if (++arg < args) {
					parsed[type].ptr = argv[arg];
					arg++;
				} else {
					parsed[type].ptr = NULL;
				}
			}
		}
	}

	if (arg == 0 && *argv[0] == '-') {
		arg = shortargparse(argv[0] + 1, type_array, parsed);
	}

	return arg;
}

/* コマンドラインの数だけ argparse を呼び出す
 */
int option_parse(
	int args,                          // argvの要素数
	const char *argv[],                // 解析するオプション
	const struct opt_type *type_array, // オプション形式のリスト
	struct opt_parsed *parsed)         // コマンドラインの解析結果の格納先
{
	int arg;

	for (arg = 0; arg < args; ) {
		int arginc;
		arginc = argparse(args - arg, argv + arg, type_array, parsed);
		if (arginc) {
			arg += arginc;
		} else {
			break;
		}
	}

	return arg;
}
