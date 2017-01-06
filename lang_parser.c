#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SEPARATOR	','
#define MAX_LINE_LEN	1048575
#define MAX_NUM_COL	65535

__dead void
usage(void)
{
	extern char	*__progname;
	fprintf(stderr, "usage: %s [ch]\n",
		__progname);
	exit(1);
}

const char *const *
separate(int separator)
{
	static char	 buf[MAX_LINE_LEN + 1];
	static const char *col[MAX_NUM_COL];
	int		 ibuf, icol, ch, is_first, close_char;
	ibuf = icol = 0;
	is_first = 1;
	close_char = 0;
	col[icol++] = buf;
	while ((ch = getchar()) >= 0 && ch != '\n') {
retry:
		if (ibuf >= MAX_LINE_LEN)
			errx(1, "line is too long");
		else if (icol >= MAX_NUM_COL)
			errx(1, "too many columns");
		else if (ch == '\r')
			continue;

		if (is_first && ch == '"') {
			close_char = ch;
			ch = getchar();
		}
		is_first = 0;
		if (close_char && ch == close_char) {
			ch = getchar();
			if (ch == close_char) {
				buf[ibuf++] = (char)ch;
			} else {
				close_char = 0;
				goto retry;
			}
		} else if (close_char) {
			buf[ibuf++] = (char)ch;
		} else if (ch == separator) {
			buf[ibuf++] = 0;
			col[icol++] = buf + ibuf;
		} else if (ch == '\n') {
			break;
		} else {
			buf[ibuf++] = (char)ch;
		}
	}
	if (close_char)
		errx(1, "expecting '\"' before EOL");
	buf[ibuf++] = 0;
	col[icol++] = 0;
	if (ibuf == 1)
		col[0] = 0;
	if (!col[0] && ch < 0)
		return 0;
	return col;
}

void
quote(const char *str)
{
	int i;
	if (!str) {
		putchar('0');
		return;
	}
	putchar('"');
	for (i = 0; str[i]; ++i) {
		int ch = (unsigned char)str[i];
		if (ch == '"')
			putchar('\\');
		if (ch < ' ' || ch >= 127)
			printf("\\%03o", ch);
		else
			putchar(ch);
	}
	putchar('"');
}

void
print_string_array(int mode, const char *prefix,
    const char *name, const char *const *val)
{
	int i;
	if (mode == 'h')
		printf("extern ");
	printf("const char *%s", prefix);
	for (i = 0; name[i]; ++i) {
		int ch = (unsigned char)name[i];
		if ((ch >> 6) == 2)
			continue;
		if (isalnum(ch))
			putchar(tolower(ch));
		else
			putchar('_');
	}
	printf("[LANG_COUNT]");
	if (mode == 'h') {
		puts(";");
	} else {
		puts(" = {");
		for (i = 0; val[i]; ++i) {
			putchar('\t');
			quote(val[i]);
			if (val[i+1])
				putchar(',');
			putchar('\n');
		}
		puts("};\n");
	}
}

int
main(int argc, char **argv)
{
	int	 mode;
	int	 ncol;
	const char *const *col;

	if (argc != 2 || strlen(argv[1]) != 1)
		usage();
	mode = *argv[1];
	if (mode != 'c' && mode != 'h')
		usage();

	// head
	if (mode == 'h') {
		puts(
			"#ifndef __LANG_H__\n"
			"#define __LANG_H__\n"
			"\n"
			"#ifdef __cplusplus\n"
			"extern \"C\" {\n"
			"#endif\n"
			"");
	} else {
		puts(
			"#include <lang.h>\n"
			"");
	}

	// body
	col = separate(SEPARATOR);
	ncol = 0;
	while (col[ncol])
		++ncol;
	if (mode == 'h') {
		printf(
			"#define LANG_COUNT\t%d\n"
			"", ncol);
	}
	print_string_array(mode, "lang_code", "", col);
	col = separate(SEPARATOR);
	print_string_array(mode, "lang_dir", "", col);
	col = separate(SEPARATOR);
	print_string_array(mode, "lang_comma", "", col);
	col = separate(SEPARATOR);
	print_string_array(mode, "lang_colon", "", col);
	col = separate(SEPARATOR);
	print_string_array(mode, "lang_period", "", col);
	col = separate(SEPARATOR);
	print_string_array(mode, "lang_excla", "", col);
	col = separate(SEPARATOR);
	print_string_array(mode, "lang_quest", "", col);
	while ((col = separate(SEPARATOR)) && col[0])
		print_string_array(mode, "str_", col[0], col);

	// tail
	if (mode == 'h') {
		puts(
			"\n"
			"#ifdef __cplusplus\n"
			"}\n"
			"#endif\n"
			"\n"
			"#endif\n"
			"");
	} else {
	}

	return 0;
}
