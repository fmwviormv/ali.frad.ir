#include <sys/stat.h>
#include <sys/syslimits.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char path[PATH_MAX + 1];
char last_path[PATH_MAX + 1];
int last_count;
const char *bin_prefix = "bin/";

__dead void
usage(void)
{
	extern char	*__progname;
	fprintf(stderr, "usage: %s [ch] basepath\n",
		__progname);
	exit(1);
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
quote_nonempty_file(const char *path)
{
	char buf[32];
	int nbuf;
	FILE *file = fopen(path, "rb");
	while ((nbuf = fread(buf, 1, sizeof(buf), file))) {
		int i;
		printf("\n\t\"");
		for (i = 0; i < nbuf; ++i) {
			int ch = (unsigned char)buf[i];
			if (ch == '"' || ch < 32 || ch >= 127)
				putchar('\\'); // must quote

			if (ch == '\t')
				putchar('t');
			else if (ch == '\r')
				putchar('r');
			else if (ch == '\n')
				putchar('n');
			else if (ch < ' ' || ch >= 127)
				printf("%03o", ch);
			else
				putchar(ch);
		}
		putchar('"');
	}
	if (ferror(file))
		errx(1, "error while reading file '%s'",
		    path);
}

int
common_prefix(const char *s1, const char *s2)
{
	int i;
	for (i = 0; s1[i]; ++i) {
		if (s1[i] != s2[i])
			return 0;
		if (s1[i] == '/')
			return 1;
	}
	errx(1, "different files share same path");
}

void
close_last(int mode)
{
	if (!last_count)
		return;
	if (mode == 'h')
		printf("[%d];\n", last_count);
	else
		puts("\n};");
	last_count = 0;
}

void
print_type_name(int mode, int isarray, const char *prefix,
    const char *path, int ignore_last_part) {
	int i, len;
	if (ignore_last_part)
		len = strchr(path, '/') - path;
	else
		len = strlen(path);
	if (mode == 'c') {
		printf("\n// ");
		fwrite(path, 1, len, stdout);
		putchar('\n');
	}

	if (mode == 'h')
		printf("extern ");
	printf("const char ");
	if (isarray)
		putchar('*');
	for (i = 0; prefix[i]; ++i) {
		int ch = (unsigned char)prefix[i];
		if ((ch >> 6) == 2)
			continue;
		if (isalnum(ch))
			putchar(tolower(ch));
		else
			putchar('_');
	}
	for (i = 0; i < len; ++i) {
		int ch = (unsigned char)path[i];
		if ((ch >> 6) == 2)
			continue;
		if (isalnum(ch))
			putchar(tolower(ch));
		else
			putchar('_');
	}
}

long long
get_index(const char *path)
{
	int i, start = 0;
	long long res = 0;
	for (i = 0; path[i]; ++i)
		if (path[i] == '/')
			start = i;
	while (path[start] && !isdigit(path[start]))
		++start;
	for (i = start; isdigit(path[i]); ++i)
		res = res * 10 + (path[i] & 15);
	return res;
}

void
print_file(int mode, int baselen, const char *prefix,
    const char *path)
{
	int i;
	struct stat sb;
	while (path[baselen] == '/')
		++baselen;
	stat(path, &sb);
	if (common_prefix(path + baselen, bin_prefix)) {
		close_last(mode);
		print_type_name(mode, 0, prefix,
		    path + baselen, 0);
		printf("[%lld]", (long long)sb.st_size);
		if (mode == 'c' && sb.st_size) {
			printf(" =");
			quote_nonempty_file(path);
		}
		puts(";");
	} else if (common_prefix(path + baselen, last_path)) {
		long long index = get_index(path + baselen);
		if (mode == 'h') {
			last_count = index + 1;
		} else {
			while (last_count < index)
				printf(",\n\t0" + !last_count++);
			if (last_count++)
				putchar(',');
			printf("\n\t// %lld: %s",
			    last_count - 1, path + baselen);
			if (sb.st_size)
				quote_nonempty_file(path);
			else
				printf("\n\t\"\"");
		}
	} else if (strchr(path + baselen, '/')) {
		long long index = get_index(path + baselen);
		close_last(mode);
		print_type_name(mode, 1, prefix,
		    path + baselen, 1);
		if (mode == 'h') {
			last_count = index + 1;
		} else {
			last_count = 0;
			printf("[] = {");
			while (last_count < index)
				printf(",\n\t0" + !last_count++);
			if (last_count++)
				putchar(',');
			printf("\n\t// %lld: %s",
			    last_count - 1, path + baselen);
			if (sb.st_size)
				quote_nonempty_file(path);
			else
				printf("\n\t\"\"");
		}
	} else {
		close_last(mode);
		print_type_name(mode, 0, prefix,
		    path + baselen, 0);
		printf("[%lld + 1]", (long long)sb.st_size);
		if (mode == 'c' && sb.st_size) {
			printf(" =");
			quote_nonempty_file(path);
		}
		puts(";");
	}
	strlcpy(last_path, path + baselen, sizeof(last_path));
}

int
main(int argc, char **argv)
{
	int	 mode;
	int	 baselen;

	if (argc != 3 || strlen(argv[1]) != 1)
		usage();
	mode = *argv[1];
	if (mode != 'c' && mode != 'h')
		usage();
	baselen = strlen(argv[2]);

	// head
	if (mode == 'h') {
		puts(
			"#ifndef __RES_H__\n"
			"#define __RES_H__\n"
			"\n"
			"#ifdef __cplusplus\n"
			"extern \"C\" {\n"
			"#endif\n"
			"");
	} else {
		puts(
			"#include <res.h>\n"
			"");
	}

	// body
	while (fgets(path, sizeof(path), stdin)) {
		int len = strlen(path);
		if (len > 0 && path[len-1] == '\n')
			path[len-1] = 0;
		if (!*path)
			continue;
		print_file(mode, baselen, "res_", path);
	}
	close_last(mode);

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
