#ifndef _RES_PARSER_H_
#include "parser.h"
#endif

enum {
	BLOCK_SIZE = 4096
};

int
filecmp(const char *file1, const char *file2)
{
	FILE		*f1 = fopen(file1, "rb");
	FILE		*f2 = fopen(file2, "rb");
	int		 res = f1 && f2 ? 0 : 1;

	while (res == 0) {
		int		 i, n;
		unsigned char	 b1[BLOCK_SIZE], b2[BLOCK_SIZE];
		n = fread(&b1, sizeof(*b1), BLOCK_SIZE, f1);

		if (fread(&b2, sizeof(*b2), BLOCK_SIZE, f2) != n) {
			res = 1;
		} else if (n <= 0) {
			break;
		} else {
			for (i = 0; i < n; ++i) {
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
		int		 i, n;
		unsigned char	 b[BLOCK_SIZE];
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
