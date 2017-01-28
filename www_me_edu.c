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
	fastcgi_addbody(t, str_after__s[L],
	    str_2014[L]);
	fastcgi_addbody(t, "</h4>");
	fastcgi_addbody(t, "<ul><li>");
	fastcgi_addbody(t, str_i_learned_more_about__s_and__s[L],
	    str_high_performance_computing[L],
	    str_code_quality[L]);
	fastcgi_addbody(t, "%s</li><li>", lang_comma[L]);
	fastcgi_addbody(t, str_i_developed__s_using__s_with__s_in_mind[L],
	    str_my_personal_website[L],
	    str_c_programming_language[L],
	    str_security_and_performance[L]);
	fastcgi_addbody(t, "%s</li></ul><br />", lang_period[L]);

	fastcgi_addbody(t, "<h4>");
	fastcgi_addbody(t, str_master_of_science_in__s[L],
	    str_computer_science[L]);
	fastcgi_addbody(t, "%s ", lang_comma[L]);
	fastcgi_addbody(t, str__s_to__s[L],
	    str_2012[L], str_2014[L]);
	fastcgi_addbody(t, "</h4>");
	fastcgi_addbody(t, "<p><a href=\"http://aut.ac.ir/\">");
	fastcgi_addbody(t, str__s_in__s[L],
	    str_amirkabir_university_of_technology[L],
	    str_tehran[L]);
	fastcgi_addbody(t, "</a></p><ul><li>");
	fastcgi_addbody(t, "%s</li></ul><br />", lang_period[L]);

	fastcgi_addbody(t, "<h4>");
	fastcgi_addbody(t, str_bachelor_of_science_in__s[L],
	    str_computer_science[L]);
	fastcgi_addbody(t, "%s ", lang_comma[L]);
	fastcgi_addbody(t, str__s_to__s[L],
	    str_2008[L], str_2012[L]);
	fastcgi_addbody(t, "</h4>");
	fastcgi_addbody(t, "<p><a href=\"http://aut.ac.ir/\">");
	fastcgi_addbody(t, str__s_in__s[L],
	    str_amirkabir_university_of_technology[L],
	    str_tehran[L]);
	fastcgi_addbody(t, "</a></p><ul><li>");
	fastcgi_addbody(t, "%s</li></ul><br />", lang_period[L]);

	fastcgi_addbody(t, "<h4>");
	fastcgi_addbody(t, str_before__s[L],
	    str_2008[L]);
	fastcgi_addbody(t, "</h4>");
	fastcgi_addbody(t, "<ul><li>");
	fastcgi_addbody(t, "%s</li></ul><br />", lang_period[L]);

	cgi_html_tail(t);
}

