#include <string.h>

#include "fastcgi.h"
#include "res.h"

static void	 process(struct mythread *);

void
www_me_work(struct mythread *t, const char *path)
{
	if (strcmp(path, "") == 0)
		process(t);
	else
		cgi_notfound(t);
}

static void
process(struct mythread *t)
{
	int		 L = t->lang;
	cgi_html_begin(t, "%s", "%s",
	    str_my_works[L], str_my_works[L]);
	fastcgi_addbody(t, "<style>%s", res_css_common);
	fastcgi_trim_end(t);
	fastcgi_addbody(t, "</style>");
	cgi_html_head(t, "%s", str_my_works[L]);
	cgi_html_tail(t);
}

