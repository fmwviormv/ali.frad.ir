#include <string.h>

#include "fastcgi.h"

static void	 process(struct mythread *);

void
www_utils(struct mythread *t, const char *path)
{
	if (strcmp(path, "") == 0)
		process(t);
	else
		cgi_notfound(t);
}

static void
process(struct mythread *t)
{
}

