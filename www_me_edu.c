#include <string.h>

#include "fastcgi.h"
#include "lang.h"
#include "res.h"

static void	 process(struct mythread *);

void
www_me_edu(struct mythread *t, const char *path)
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
	    str_my_educations[L], str_my_educations[L]);
	fastcgi_addbody(t, "<style>%s", res_css_common);
	fastcgi_trim_end(t);
	fastcgi_addbody(t, "</style>");
	cgi_html_head(t, "%s", str_my_educations[L]);

	fastcgi_addbody(t, "<h4>");
	fastcgi_addbody(t, str_bachelor_of_science_in__s[L],
	    str_computer_science[L]);
	fastcgi_addbody(t, "%s ", lang_comma[L]);
	fastcgi_addbody(t, str_from__s_to__s[L],
	    str_2008[L], str_2012[L]);
	fastcgi_addbody(t, "</h4>");

	fastcgi_addbody(t, "<h4>");
	fastcgi_addbody(t, str_master_of_science_in__s[L],
	    str_computer_science[L]);
	fastcgi_addbody(t, "%s ", lang_comma[L]);
	fastcgi_addbody(t, str_from__s_to__s[L],
	    str_2012[L], str_2014[L]);
	fastcgi_addbody(t, "</h4>");

	cgi_html_tail(t);
}

