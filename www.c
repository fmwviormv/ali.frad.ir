#include "fastcgi.h"

void
www(struct mythread *t, const char *path)
{
	if (*path) {
		if (cgi_path(&path, "os"))
			www_os(t, path);
		else if (cgi_path(&path, "games"))
			www_games(t, path);
		else if (cgi_path(&path, "utils"))
			www_utils(t, path);
		else
			cgi_notfound(t);
		return;
	}
}

