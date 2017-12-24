#include <string.h>

#include "fastcgi.h"
#include "res.h"

static void	 process(struct mythread *);

void
www_me(struct mythread *t, const char *path)
{
	if (cgi_path(&path, "edu"))
		www_me_edu(t, path);
	else if (cgi_path(&path, "work"))
		www_me_work(t, path);
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

