/* FILE : putimg.c
 * DATE : 2008-11-05
 * VER  : 0.0.1
 * (C)  : T.Kato
 * DESC : putimgコマンド
 */

#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "cmdparse.h"

const struct opt_style optstyle_to[] = {
	{ OPTSTYLE_NEXT, "-t" },
	OPT_STYLE_NULL
};
const struct opt_type cmdoption[] = {
	{ optstyle_to },
	OPT_TYPE_NULL
};

int main(int _argc, const char *_argv[])
{
	const char *paths[2] = { NULL };
	int srcfd, destfd;
	int argc;
	const char **argv;
	int i;
	off_t destto;
	struct opt_parsed cmdparsed[1] = { { {0} } };

	argc = _argc - 1;
	argv = _argv + 1;

	for (i = 0; argc > 0 && i < 2; i++) {
		int arginc;
		arginc = option_parse(argc, argv, cmdoption, cmdparsed);
		argc -= arginc;
		argv += arginc;
		if (argc > 0) {
			argc--;
			paths[i] = *argv++;
		}
	}
	if (paths[1] == NULL) {
		abort();
	}

//	if (argc > 3) {
		srcfd = open(paths[0], O_RDONLY);
		if (srcfd == -1) {
			abort();
		}
		destfd = open(paths[1], O_WRONLY);
		if (destfd == -1) {
			close(srcfd);
			abort();
		}
		if (cmdparsed[0].ptr != NULL) {
			destto = strtoll(cmdparsed[0].ptr, NULL, 0);
		}
//	} else {
//		abort();
//	}

	lseek(destfd, destto, SEEK_SET);

	for (;;) {
		char buf[1024];
		ssize_t len;
		len = read(srcfd, buf, sizeof buf);
		write(destfd, buf, len);
		if (len < sizeof buf) {
			break;
		}
	}

	close(srcfd);
	close(destfd);

	return 0;
}
