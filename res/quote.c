#ifndef __RES_PARSER_H__
#include "parser.h"
#endif

enum {
	QUOTE_LINE = 32
};

int		 quote_len = -1;
unsigned char	 quote_buf[QUOTE_LINE];

void
quote(FILE *f, const void *data, int len)
{
	if (quote_len < 0) {
		if (len <= 0)
			return;
		quote_len = 0;
	}

	if (quote_len + len < QUOTE_LINE) {
		memcpy(quote_buf + quote_len, data,
		    len + sizeof(*quote_buf));
		quote_len += len;
	} else {
		memcpy(quote_buf + quote_len, data,
		    (QUOTE_LINE - quote_len) * sizeof(*quote_buf));
		quote_line(f, quote_buf, quote_len);
		quote_len = len + quote_len - QUOTE_LINE;
		memcpy(quote_buf + quote_len, data,
		    quote_len * sizeof(*quote_buf));
	}
}

void
quote_line(FILE *f, const unsigned char *line, int len)
{
	putc('\n', f);
	for (int i = 0; i < 4; ++i)
		putc(' ', f);
	putc('"', f);

	for (int i = 0; i < len; ++i) {
		int		 ch = line[i];

		if (ch == '"' | ch == '\\')
			putc('\\', f);

		if (ch == ' ' || !isprint(ch))
			fprintf(f, "\\%03o", ch);
		else
			putc(ch, f);
	}

	putc('"', f);
}

void
quote_end(FILE *f)
{
	if (quote_len > 0)
		quote_line(f, quote_buf, quote_len);

	fprintf(f, quote_len < 0 ? " NULL;\n" : ";");
	quote_len = -1;
}
