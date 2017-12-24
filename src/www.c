#include <string.h>

#include "fastcgi.h"
#include "res.h"

static void	 process(struct mythread *);

void
www(struct mythread *t, const char *path)
{
	if (cgi_path(&path, "stat"))
		www_stat(t, path);
	else if (cgi_path(&path, "me"))
		www_me(t, path);
	else if (cgi_path(&path, "os"))
		www_os(t, path);
	else if (cgi_path(&path, "games"))
		www_games(t, path);
	else if (cgi_path(&path, "utils"))
		www_utils(t, path);
	else if (strcmp(path, "") == 0)
		process(t);
	else
		cgi_notfound(t);
}

static void
process(struct mythread *t)
{
	const RES	 header[] = {
		res.http.header.www.html,
		RES_NULL
	},		 content[] = {
		res.html.www.index,
		RES_NULL
	};

	fastcgi_addhead(t, header);
	fastcgi_addbody(t, content);
}

