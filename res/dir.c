#ifndef __RES_PARSER_H__
#include "parser.h"
#endif

enum {
	MAXARRAYSIZE = 1000000
};

void
dirscan(char *parent_path, const char *dir, struct res *res)
{
	int		 ndir = 0;
	int		 nreg = 0;
	DIR		*dirp;
	static char	 parent_copy[PATH_MAX];
	static char	 path[PATH_MAX];

	snprintf(parent_copy, sizeof(parent_copy), "%s", parent_path);

	if (snprintf(path, sizeof(path), "%s%s%s", parent_copy,
	    dir[0] != 0 ? "/" : "", dir) >= (int)sizeof(path))
		errx(1, "too long path `%s/%s'", path, dir);

	snprintf(res->name, sizeof(res->name), "%s", dir);
	snprintf(res->path, sizeof(res->path), "%s", path);
	dirp = opendir(res->path);

	if (dirp != NULL) {
		struct dirent	*dp;
		while ((dp = readdir(dirp)) != NULL) {
			const char	*ign = dirignore(path, dp, '.');

			if (ign != NULL)
				fprintf(stderr, "ignoring `%s/%s' %s\n",
				    path, dp->d_name, ign);
			else if (dp->d_type == DT_DIR)
				++ndir;
			else if (dp->d_type == DT_REG)
				++nreg;
		}
		closedir(dirp);
	}

	dirp = opendir(res->path);

	if (ndir > 0) {
		struct dirent	*dp;
		res->nchild = ndir;
		res->child = calloc(res->nchild, sizeof(*res->child));
		ndir = 0;
		while ((dp = readdir(dirp)) != NULL) {
			if (dirignore(path, dp, 0))
				continue;
			else if (dp->d_type != DT_DIR)
				fprintf(stderr, "ignoring `%s/%s' %s\n",
				    path, dp->d_name, "regular file");
			else if (++ndir > res->nchild)
				break;
			else
				dirscan(path, dp->d_name,
				    &res->child[ndir - 1]);
		}

		if (ndir != res->nchild)
			errx(1, "directory change during scan");
	} else {
		struct dirent	*dp;
		res->nchild = nreg;
		res->child = NULL;
		nreg = 0;
		while ((dp = readdir(dirp)) != NULL) {
			if (dirignore(path, dp, '.'))
				continue;
			else if (dp->d_type != DT_REG ||
			    ++nreg > res->nchild)
				nreg = res->nchild + 1;
			else
				dirscan_res(path, dp->d_name, res);
		}

		if (nreg != res->nchild)
			errx(1, "directory change during scan");

		res->nchild = 0;
	}

	if (dirp) closedir(dirp);
	dirup(path);
}

void
dirscan_res(const char *path, const char *name, struct res *res)
{
	char		 lang[MAXRESNAME + 1];
	char		*point;
	struct file	*file = NULL;

	snprintf(lang, sizeof(lang), "%s", name);
	point = strchr(lang, '.');

	if (point)
		*point = 0;

	if (strcmp(lang, "default") == 0)
		file = &res->default_file;

	for (int i = 0; i < LANG_COUNT; ++i)
	if (strcmp(lang, lang_code[i]))
		file = &res->by_lang[i];

	if (!file)
		errx(1, "%s: bad file name `%s'", path, name);
	else if (file->name[0])
		errx(1, "%s: multiple files `%s' and `%s'",
		    path, file->name, name);
	else {
		snprintf(file->name, sizeof(file->name), "%s", name);
		file->length = filelen(path, name);
	}
}

void
dirup(char *path)
{
	int		 len = strlen(path);

	while (len > 0 && path[--len] != '/') ;

	path[len] = 0;
}

const char *
dirignore(const char *path, const struct dirent *dp, int end_char)
{
	const char	*message = NULL;

	if (dp->d_namlen <= 0)
		message = "due to invalid name";
	else if (dp->d_name[0] == '.')
		message = "hidden file";
	else if (dp->d_namlen > MAXRESNAME)
		message = "due to long name";
	else if (strlen(path) + 1 + dp->d_namlen >= PATH_MAX)
		message = "due to long path";
	else if (dp->d_type != DT_DIR && dp->d_type != DT_REG)
		message = "due to file type";
	else if (isdigit(dp->d_name[0])) {
		long long	 index = 0;
		for (int i = 0; i < (int)dp->d_namlen; ++i) {
			const int	 ch = dp->d_name[i];
			index = index * 10 + (ch - '0');

			if (ch == end_char)
				break;
			else if (!isdigit(ch))
				message = "due to invalid index";
			else if (index >= MAXARRAYSIZE)
				message = "due to large index";
		}
	} else {
		for (int i = 0; i < (int)dp->d_namlen; ++i) {
			const int	 ch = dp->d_name[i];

			if (ch == end_char)
				break;
			else if (ch != '_' && !isalnum(ch))
				message = "due to invalid character";
			else if (i == 0 && isdigit(ch))
				message = "due to starting with digit";
		}
	}

	return message;
}

void
dirsort(struct res *res)
{
	for (int i = 0; i < res->nchild; ++i)
		dirsort(&res->child[i]);

	qsort(res->child, res->nchild, sizeof(*res->child), dircmp);
}

int
dircmp(const void *x, const void *y)
{
	const struct res	*px = x, *py = y;

	if (isdigit(px->name[0]) && isdigit(py->name[0])) {
		int		 ix = 0, iy = 0;
		for (int i = 0; px->name[i]; ++i)
			ix = ix * 10 + (px->name[i] - '0');
		for (int i = 0; py->name[i]; ++i)
			iy = iy * 10 + (py->name[i] - '0');
		return ix < iy ? -1 : ix > iy ? 1 : 0;
	} else {
		return strcmp(px->name, py->name);
	}
}

int
dirpcmp(const void *x, const void *y)
{
	return dircmp(*(const void *const *)x, *(const void *const *)y);
}
