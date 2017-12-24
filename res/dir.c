#ifndef __RES_PARSER_H__
#include "parser.h"
#endif

enum {
	MAXARRAYSIZE = 1000000
};

void
dirscan(const char *path, const char *dir, struct res *res)
{
	int		 ndir = 0;
	int		 nreg = 0;
	DIR		*dirp;

	if (snprintf(res->path, sizeof(res->path), "%s%s%s", path,
	    dir[0] != 0 ? "/" : "", dir) >= (int)sizeof(res->path))
		errx(1, "too long path `%s/%s'", path, dir);

	snprintf(res->name, sizeof(res->name), "%s", dir);
	dirp = opendir(res->path);

	if (dirp != NULL) {
		struct dirent	*dp;
		while ((dp = readdir(dirp)) != NULL) {
			if (dirignore(stderr, res->path, dp))
				continue;
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
			if (dirignore(NULL, res->path, dp))
				continue;
			else if (dp->d_type != DT_DIR)
				fprintf(stderr, "ignoring `%s/%s' %s\n",
				    path, dp->d_name, "regular file");
			else if (++ndir > res->nchild)
				break;
			else
				dirscan(res->path, dp->d_name,
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
			if (dirignore(NULL, res->path, dp))
				continue;
			else if (dp->d_type != DT_REG ||
			    ++nreg > res->nchild)
				nreg = res->nchild + 1;
			else
				dirscan_res(res, dp->d_name);
		}

		if (nreg != res->nchild)
			errx(1, "directory change during scan");

		res->nchild = 0;
	}

	if (dirp) closedir(dirp);
}

void
dirscan_res(struct res *res, const char *name)
{
	char		 lang[MAXRESNAME + 1];
	char		*point;
	const char	*path = res->path;
	struct file	*file = NULL;

	snprintf(lang, sizeof(lang), "%s", name);
	point = strchr(lang, '.');

	if (point)
		*point = 0;

	if (strcmp(lang, "default") == 0)
		file = &res->default_file;

	for (int i = 0; i < LANG_COUNT; ++i)
	if (strcmp(lang, lang_code[i]) == 0)
		file = &res->by_lang[i];

	if (!file)
		errx(1, "%s: bad file name `%s'", path, name);
	else if (file->name[0])
		errx(1, "%s: multiple files `%s' and `%s'",
		    path, file->name, name);
	else
		snprintf(file->name, sizeof(file->name), "%s", name);
}

int
dirignore(FILE *f, const char *path, const struct dirent *dp)
{
	const char	*name = dp->d_name;
	const char	*message = NULL;
	int		 ignore = 0;

	if (dp->d_namlen <= 0)
		message = "due to invalid name";
	else if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
		ignore = 1;
	else if (name[0] == '.')
		message = "hidden file";
	else if (dp->d_namlen > MAXRESNAME)
		message = "due to long name";
	else if (strlen(path) + 1 + dp->d_namlen >= PATH_MAX)
		message = "due to long path";
	else if (dp->d_type != DT_DIR && dp->d_type != DT_REG)
		message = "due to file type";
	else if (isdigit(name[0])) {
		long long	 index = 0;
		for (int i = 0; i < (int)dp->d_namlen; ++i) {
			const int	 ch = name[i];
			index = index * 10 + (ch - '0');

			if (ch == '.')
				break;
			else if (!isdigit(ch))
				message = "due to invalid index";
			else if (index >= MAXARRAYSIZE)
				message = "due to large index";
		}
	} else {
		for (int i = 0; i < (int)dp->d_namlen; ++i) {
			const int	 ch = name[i];

			if (ch == '.')
				break;
			else if (ch != '_' && !isalnum(ch))
				message = "due to invalid character";
			else if (i == 0 && isdigit(ch))
				message = "due to starting with digit";
		}
	}

	if (message) {
		ignore = 1;

		if (f)
			fprintf(f, "ignoring `%s/%s' %s\n",
			    path, name, message);
	}

	return ignore;
}

void
dirsort(struct res *res)
{
	qsort(res->child, res->nchild, sizeof(*res->child), dircmp);

	for (int i = 0; i < res->nchild; ++i)
		dirsort(&res->child[i]);
}

int
dircmp(const void *x, const void *y)
{
	const char	*px = ((const struct res *)x)->name;
	const char	*py = ((const struct res *)y)->name;

	if (isdigit(px[0]) && isdigit(py[0])) {
		int		 ix = 0, iy = 0;
		for (int i = 0; px[i] && px[i] != '.'; ++i)
			ix = ix * 10 + (px[i] - '0');
		for (int i = 0; py[i] && py[i] != '.'; ++i)
			iy = iy * 10 + (py[i] - '0');
		return ix < iy ? -1 : ix > iy ? 1 : 0;
	} else {
		return strcmp(px, py);
	}
}

int
dirpcmp(const void *x, const void *y)
{
	return dircmp(*(const void *const *)x, *(const void *const *)y);
}
