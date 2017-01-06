#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "fastcgi.h"
#include "lang.h"

void		 cgi_root(struct mythread *);

void
cgi_main(struct mythread *t, struct myconnect *c)
{
	const char	*path = t->path_info;
	if (*path == '/')
		++path;
	if (!*path) {
		cgi_root(t);
	} else {
		cgi_path(&path, lang_code[c->lang]);
		www(t, path);
	}
	fastcgi_end(t, c);
}

int
cgi_path(const char **path, const char *s)
{
	int		 len = strlen(s);
	int		 ch;
	if (strncmp(*path, s, len) != 0)
		return 0;
	ch = (*path)[len];
	if (ch && ch != '/')
		return 0;
	*path += len;
	if (**path == '/')
		++*path;
	return 1;
}

void
cgi_root(struct mythread *t)
{
}

void
cgi_notfound(struct mythread *t)
{
	int		 L = t->lang;
	fastcgi_addhead(t, "Status: %d\n", 404);
	cgi_html_begin(t, str_not_found[L], NULL);
	cgi_html_head(t);
	fastcgi_addbody(t, "<h3>%s</h3>"
	    "<h5>%s%s</h5>",
	    str_not_found[L],
	    str_requested_resource_not_found[L],
	    lang_period[L]);
	cgi_html_tail(t);
}

void
cgi_html_begin(struct mythread *t, const char *title_fmt,
    const char *desc_fmt, ...)
{
	int		 L = t->lang;
	va_list		 ap;
	fastcgi_addhead(t, "Content-Type: " MIME_HTML "\n");
	fastcgi_addbody(t, "<!DOCTYPE html>\n"
	    "<html dir=\"%s\" lang=\"%s\">"
	    "<head>"
	    "<meta charset=\"utf-8\">"
	    "<meta http-equiv=\"x-ua-compatible\" content=\"ie=edge\">"
	    "<meta name=\"viewport\" content=\"width=device-width"
	    ", initial-scale=1\">",
	    lang_dir[L], lang_code[L]);
	va_start(ap, desc_fmt);
	if (title_fmt) {
		fastcgi_addbody(t, "<title>");
		t->body_len += vsnprintf(t->body + t->body_len,
		    sizeof(t->body) - t->body_len, title_fmt, ap);
		fastcgi_addbody(t, "</title>");
	}
	if (desc_fmt) {
		fastcgi_addbody(t, "<meta name=\"description\" content=\"");
		t->body_len += vsnprintf(t->body + t->body_len,
		    sizeof(t->body) - t->body_len, title_fmt, ap);
		fastcgi_addbody(t, "\">");
	}
	va_end(ap);
}

void
cgi_html_head(struct mythread *t)
{
	fastcgi_addbody(t, "</head>"
	    "<body>");
}

void
cgi_html_tail(struct mythread *t)
{
	fastcgi_addbody(t, "</body>"
	    "</html>");
}

void
cgi_minify2(struct mythread *t, const void *data, int len)
{
}

void
cgi_image2(struct mythread *t, const void *data, int len)
{
}

