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
	int		 L = t->lang;
	cgi_html_begin(t, str__s_s_personal_website[L],
	    str__s_s_personal_website[L],
	    str_ali_farzanrad[L],
	    str_ali_farzanrad[L]);
	fastcgi_addbody(t, "<style>%s", res_css_common);
	fastcgi_trim_end(t);
	fastcgi_addbody(t, "</style>");
	cgi_html_head(t, str__s_s_personal_website[L],
	    str_ali_farzanrad[L]);
	fastcgi_addbody(t, "<p>%s%s</p><ul>",
	    str_content_list[L],
	    lang_colon[L]);

	fastcgi_addbody(t, "<li><a href=\"me/\">%s</a></li>",
	    str_about_me[L]);

	fastcgi_addbody(t, "</ul>");
	cgi_html_tail(t);
}

