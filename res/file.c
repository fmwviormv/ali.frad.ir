#ifndef __RES_PARSER_H__
#include "parser.h"
#endif

enum {
	BLOCK_SIZE = 4096
};

int
filelen(const char *path, const char *name)
{
	int		 len = strlen(path);
	static char	 fullpath[2 * (PATH_MAX)];
	FILE		*file;
	long		 filelen;

	snprintf(fullpath, sizeof(fullpath), "%s%s%s",
	    path, len > 0 ? "/" : "", name);
	file = fopen(fullpath, "rb");

	if (!file)
		errx(1, "could not open file `%s'", fullpath);

	fseek(file, 0, SEEK_END);
	filelen = ftell(file);
	fclose(file);

	if (filelen != (int)filelen)
		errx(1, "file is too big `%s'", fullpath);

	return (int)filelen;
}

int
filecmp(const char *file1, const char *file2)
{
	FILE		*f1 = fopen(file1, "rb");
	FILE		*f2 = fopen(file2, "rb");
	int		 res = f1 && f2 ? 0 : 1;

	while (res == 0) {
		size_t		 n;
		static unsigned char b1[BLOCK_SIZE], b2[BLOCK_SIZE];
		n = fread(&b1, sizeof(*b1), BLOCK_SIZE, f1);

		if (fread(&b2, sizeof(*b2), BLOCK_SIZE, f2) != n) {
			res = 1;
		} else if (n <= 0) {
			break;
		} else if (n <= BLOCK_SIZE) {
			for (int i = 0; i < (int)n; ++i) {
				if (b1[i] != b2[i])
					res = 1;
			}
		}
	}

	if (f1) fclose(f1);
	if (f2) fclose(f2);

	return res;
}

int
filecpy(const char *src, const char *dest)
{
	FILE		*f1 = fopen(src, "rb");
	FILE		*f2 = fopen(dest, "wb");
	int		 res = f1 && f2 ? 0 : 1;

	while (res == 0) {
		size_t		 n;
		static unsigned char b[BLOCK_SIZE];
		n = fread(&b, sizeof(*b), BLOCK_SIZE, f1);

		if (n <= 0)
			break;
		else if (fwrite(&b, sizeof(*b), n, f2) != n)
			res = 1;
	}

	if (f1) fclose(f1);
	if (f2) fclose(f2);

	return res;
}
