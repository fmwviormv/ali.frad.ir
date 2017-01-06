#include <stdio.h>
#include <string.h>

#include "fastcgi.h"
#include "lang.h"

void		 cgi_root(struct mythread *);

void
cgi_main(struct mythread *t, struct myconnect *c)
{
	const char	*path = t->path_info;
	int		 lang_len = strlen(lang_code[c->lang]);
	if (*path == '/')
		++path;
	if (!*path) {
		cgi_root(t);
	} else {
		cgi_path(&path, lang_code[c->lang]);
		www(t, path);
	}
	fastcgi_end(t, c);
}

int
cgi_path(const char **path, const char *s)
{
	int		 len = strlen(s);
	if (strncmp(*path, s, len) != 0)
		return 0;
	*path += len;
	if (**path == '/')
		++*path;
	return 1;
}

void
cgi_root(struct mythread *t)
{
}

void
cgi_notfound(struct mythread *t)
{
}

