#include "fastcgi.h"

void
www_utils(struct mythread *t, const char *path)
{
	if (*path) {
		cgi_notfound(t);
		return;
	}
}

