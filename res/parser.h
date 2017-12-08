#define _RES_PARSER_H_

#include <sys/stat.h>
#include <sys/syslimits.h>
#include <sys/types.h>

#include <ctype.h>
#include <dirent.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum {
	MAXRESNAME = 31
};

enum {
	LANG_EN = 0,
	LANG_FA,
	LANG_COUNT
};

const char *lang_code[LANG_COUNT];

struct res {
	char		 name[MAXRESNAME + 1];
	int		 is_resdir;
	int		 nchild;
	struct res	*child;
	const char	*default_file;
	const char	*by_lang[LANG_COUNT];
};

struct res	*dirscan(char *, const char *, struct res *);
void		 dircat(char *, const char *);
void		 dirup(char *);
const char	*dirignore(const char *, const struct dirent *, int);
int		 filecmp(const char *, const char *);
int		 filecpy(const char *, const char *);

