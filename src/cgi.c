#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fastcgi.h"
#include "res.h"

const char	*cgi_mainpath(const struct mythread *);
void		 cgi_root(struct mythread *);

void
cgi_main(struct mythread *t, struct myconnect *c)
{
	const char	*path = cgi_mainpath(t);

	if (!path)
		cgi_root(t);
	else
		www(t, path);

	fastcgi_end(t, c);
}

int
cgi_path(const char **path, const char *s)
{
	int		 len = strlen(s);
	if (strncmp(*path, s, len) != 0 || (*path)[len] != '/')
		return 0;
	*path += len + 1;
	return 1;
}

const char *
cgi_mainpath(const struct mythread *t)
{
	const char	*path = t->path_info;
	const char	*slash = strchr(path, '/');
	const char	*result;

	if (*path == '/')
		++path;

	if (!*path)
		result = NULL;
	else if (t->lang < 0)
		result = path;
	else if (slash)
		result = slash + 1;
	else
		result = path + strlen(path);

	return result;
}

void
cgi_root(struct mythread *t)
{
	const RES	 header[] = {
		res.http.header.www.html,
		RES_NULL
	},		 content[] = {
		res.html.cgi.root,
		RES_NULL
	};

	fastcgi_addhead(t, header);
	fastcgi_addbody(t, content);
}

void
cgi_notfound(struct mythread *t)
{
	const RES	 header[] = {
		res.http.header.cgi.notfound,
		RES_NULL
	},		 content[] = {
		res.html.cgi.notfound,
		RES_NULL
	};

	fastcgi_addhead(t, header);
	fastcgi_addbody(t, content);
}
