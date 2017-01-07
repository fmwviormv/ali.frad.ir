#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fastcgi.h"
#include "lang.h"

static void	 cgi_root(struct mythread *);

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

static void
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
		if (t->body_len >= 0)
			t->body_len += vsnprintf(t->body + t->body_len,
			    sizeof(t->body) - t->body_len, title_fmt, ap);
		fastcgi_addbody(t, "</title>");
	}
	if (desc_fmt) {
		fastcgi_addbody(t, "<meta name=\"description\" content=\"");
		if (t->body_len >= 0)
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
cgi_binary2(struct mythread *t, const void *data, int len)
{
	int		 safe_len;
	if (t->body_len < 0)
		return;
	if (t->body_len + len < (int)sizeof(t->body))
		safe_len = len;
	else
		safe_len = (int)sizeof(t->body) - t->body_len - 1;
	memcpy(t->body + t->body_len, data, safe_len);
	if (safe_len == len) {
		t->body_len += len;
		t->body[t->body_len] = 0;
	} else
		t->body_len = -1;
}

void
cgi_base64_2(struct mythread *t, const void *data, int len)
{
	static const char base64[64] =
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "abcdefghijklmnopqrstuvwxyz"
	    "0123456789" "+/";
	int		 i, value, bits;
	char		*p, *end;
	if (t->body_len < 0)
		return;
	p = t->body + t->body_len;
	end = t->body + sizeof(t->body) - 1;
	value = bits = 0;
	for (i = 0; i < len; ++i) {
		int byte = ((const unsigned char *)data)[i];
		value += byte >> (bits += 2);
		if (p >= end) {
			t->body_len = -1;
			return;
		}
		*p++ = base64[value & 63];
		value = byte << (6 - bits);
		if (bits < 6)
			continue;
		if (p >= end) {
			t->body_len = -1;
			return;
		}
		*p++ = base64[value & 63];
		value = bits = 0;
	}
	*p = 0;
	t->body_len = p - t->body;
	if (bits > 0) {
		fastcgi_addbody(t, "%c%s",
		    base64[value & 63],
		    bits == 2 ? "==" : "=");
	}
}

void
cgi_image2(struct mythread *t, const void *data, int len)
{
	const unsigned char	*udata = data;
	uint32_t		 magic = 0;
	if (len >= 0)
		magic = (udata[0] << 24) + (udata[1] << 16) +
		    (udata[2] << 8) + udata[3];
	fastcgi_addbody(t, "data:%s;base64,",
	    magic == MAGIC_PNG ? "image/png" : "image/jpeg");
	cgi_base64_2(t, data, len);
}

