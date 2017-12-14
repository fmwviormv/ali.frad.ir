#ifndef __RES_PARSER_H__
#define __RES_PARSER_H__

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

extern const char *lang_code[LANG_COUNT];
extern const char *digit[LANG_COUNT][10];

struct file {
	char		 name[MAXRESNAME + 1];
	int		 length;
};

struct res {
	char		 path[PATH_MAX];
	char		 name[MAXRESNAME + 1];
	int		 nchild;
	struct res	*child;
	struct file	 default_file;
	struct file	 by_lang[LANG_COUNT];
};

void		 dirscan(const char *, const char *, struct res *);
void		 dirscan_res(struct res *, const char *);
const char	*dirignore(const struct res *, const struct dirent *);
void		 dirsort(struct res *);
int		 dircmp(const void *, const void *);
int		 dirpcmp(const void *, const void *);
int		 filelen(const char *, const char *);
int		 filecmp(const char *, const char *);
int		 filecpy(const char *, const char *);
void		 header(const struct res *, const char *);
void		 header_rec(FILE *, int, const struct res **);
void		 header_res(FILE *, int, const struct res **);
void		 header_dir(FILE *, int, const struct res **);
void		 header_array(FILE *, int, const struct res **);
const struct res **header_child(const struct res **);
void		 source(const struct res *, const char *);
void		 quote(FILE *, const void *, int);
void		 quote_line(FILE *, const unsigned char *, int);
void		 quote_end(FILE *);

#endif /* !__RES_PARSER_H__ */
