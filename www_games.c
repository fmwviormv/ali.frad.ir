#include "fastcgi.h"

void
www_games(struct mythread *t, const char *path)
{
	if (*path) {
		cgi_notfound(t);
		return;
	}
}

