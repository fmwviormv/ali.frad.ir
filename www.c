#include <string.h>

#include "fastcgi.h"
#include "lang.h"
#include "res.h"

static void	 process(struct mythread *);

void
www(struct mythread *t, const char *path)
{
	if (cgi_path(&path, "os"))
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
	int		 L = t->lang;
	cgi_html_begin(t, str__s_s_personal_website[L],
	    str__s_s_personal_website[L],
	    str_ali_farzanrad[L],
	    str_ali_farzanrad[L]);
	fastcgi_addbody(t, "<style>%s", res_css_common);
	fastcgi_trim_end(t);
	fastcgi_addbody(t, "</style>");
	cgi_html_head(t);
	fastcgi_addbody(t, "<h3>");
	fastcgi_addbody(t, str__s_s_personal_website[L],
	    str_ali_farzanrad[L]);
	fastcgi_addbody(t, "</h3><br /><p>%s%s</p><li>",
	    str_content_list[L],
	    lang_colon[L]);
	fastcgi_addbody(t, "</li>");
	cgi_html_tail(t);
}

