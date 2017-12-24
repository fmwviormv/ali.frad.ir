#include <string.h>

#include "fastcgi.h"
#include "res.h"

static void	 process(struct mythread *);

void
www_stat(struct mythread *t, const char *path)
{
	if (strcmp(path, "") == 0)
		process(t);
	else
		cgi_notfound(t);
}

static void
process(struct mythread *t)
{
	/*
	int		 L = t->lang;
	cgi_html_begin(t, "%s", "%s",
	    str_server_status[L],
	    str_server_status[L]);
	fastcgi_addbody(t, "<style>%s", res_css_common);
	fastcgi_trim_end(t);
	fastcgi_addbody(t, "</style>");
	cgi_html_head(t, "%s", str_server_status[L]);

	fastcgi_addbody(t, "<p>%s%s %d</p>",
	    str_number_of_open_connections[L],
	    lang_colon[L],
	    t->g->num_connect);
	fastcgi_addbody(t, "<p>%s%s %d</p>",
	    str_number_of_connections_per_second[L],
	    lang_colon[L],
	    t->g->num_connect_per_sec);
	fastcgi_addbody(t, "<p>%s%s %d</p>",
	    str_number_of_connections_per_minute[L],
	    lang_colon[L],
	    t->g->num_connect_per_min);

	cgi_html_tail(t);
	*/
}

