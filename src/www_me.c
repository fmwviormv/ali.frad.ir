#include <string.h>

#include "fastcgi.h"
#include "lang.h"
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
	int		 L = t->lang;
	cgi_html_begin(t, "%s", "%s",
	    str_about_me[L], str_about_me[L]);
	fastcgi_addbody(t, "<style>%s", res_css_common);
	fastcgi_trim_end(t);
	fastcgi_addbody(t, "</style>");
	cgi_html_head(t, "%s", str_about_me[L]);

	fastcgi_addbody(t, "<p>");
	fastcgi_addbody(t, str_i_m__s[L],
	    str_ali_farzanrad[L]);
	fastcgi_addbody(t, "%s ", lang_comma[L]);
	fastcgi_addbody(t,
	    str_i_graduated_from__s_in__s_with_a__s_s_degree_in__s[L],
	    str_amirkabir_university_of_technology[L], str_tehran[L],
	    str_msc[L], str_computer_science[L]);
	fastcgi_addbody(t, "%s</p><p>", lang_period[L]);
	fastcgi_addbody(t, str_i_love__s_and__s[L],
	    "UNIX", str_c_programming_language[L]);
	fastcgi_addbody(t, "%s</p><br />", lang_period[L]);

	fastcgi_addbody(t, "<p>%s%s</p><li>",
	    str_more[L],
	    lang_colon[L]);
	fastcgi_addbody(t, "<ul>");

	fastcgi_addbody(t, "<li><a href=\"edu/\">%s</a></li>",
	    str_my_educations[L]);

	fastcgi_addbody(t, "<li><a href=\"work/\">%s</a></li>",
	    str_my_works[L]);

	fastcgi_addbody(t, "</ul>");
	cgi_html_tail(t);
}

