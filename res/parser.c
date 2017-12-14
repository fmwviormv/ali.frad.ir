#ifndef __RES_PARSER_H__
#include "parser.h"
#endif

int
main(int argc, char **argv)
{
	static char	 path[PATH_MAX];
	struct res	 res;

	if (argc != 2)
		errx(1, "use exactly 1 argument for `res' directory");

	snprintf(path, sizeof(path), "%s", argv[1]);
	dirscan(path, "", &res);
	dirsort(&res);
	header(&res, "res.c");

	if (filecmp("res.c", "res.h") == 0)
		fprintf(stderr, "no change for `res.h'");
	else
		filecpy("res.c", "res.h");

	source(&res, "res.c");

	return 0;
}
