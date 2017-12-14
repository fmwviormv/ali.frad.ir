#ifndef __RES_PARSER_H__
#include "parser.h"
#endif

void
header(const struct res *res, const char *file)
{
	FILE		*f = fopen(file, "w");
	const struct res *root_res[] = { res, NULL };

	if (!f)
		errx(1, "could not open file `%s'", file);

	fprintf(f, "#ifndef __RES_H__\n"
	    "#define __RES_H__\n"
	    "\n");
	header_rec(f, 0, root_res);
	fprintf(f, "res;\n"
	    "\n"
	    "#endif /* !__RES_H__ */\n");
	fclose(f);
	return;
}

void
header_rec(FILE *f, int indent, const struct res **res)
{
	int		 has_child = 0;
	int		 is_array = 0;

	for (int i = 0; res[i]; ++i) {
		if (res[i]->nchild)
			has_child = 1;

		if (isdigit(res[i]->name[0]))
			is_array = 1;
	}

	if (!has_child)
		header_res(f, indent, res);
	else if (!is_array)
		header_dir(f, indent, res);
	else
		header_array(f, indent, res);
}

void
header_res(FILE *f, int indent, const struct res **res)
{
	for (int i = 0; i < indent; ++i)
		putc('\t', f);

	fprintf(f, "RES %s", res[0]->name);
}

void
header_dir(FILE *f, int indent, const struct res **res)
{
	for (int i = 0; i < indent; ++i)
		putc('\t', f);

	fprintf(f, "struct {\n");

	for (int i = 0; res[i]; ++i) {
		const char	*name = res[i]->name;

		if (i != 0 && strcmp(name, res[i-1]->name) == 0)
			continue;

		header_rec(f, indent + 1, header_child(res + i));
		putc(';', f);
		putc('\n', f);
	}

	for (int i = 0; i < indent; ++i)
		putc('\t', f);

	fprintf(f, "} %s", res[0]->name);
}

void
header_array(FILE *f, int indent, const struct res **res)
{
	int		 size = 0;

	for (int i = 0; res[i]; ++i) {
		int		 index = 0;
		const char	*name = res[i]->name;

		if (!isdigit(name[0]))
			continue;

		for (int j = 0; name[j]; ++j)
			index = index * 10 + (name[j] - '0');

		if (index >= size)
			size = index + 1;
	}

	header_rec(f, indent + 1, header_child(res));
	fprintf(f, "[%d]", size);
}

const struct res **
header_child(const struct res **parent)
{
	size_t		 size = 0;
	const struct res **res;
	const char	*name = NULL;

	if (parent[0] && !isdigit(parent[0]->name[0]))
		name = parent[0]->name;

	for (int i = 0; parent[i]; ++i) {
		if (name != NULL && strcmp(parent[i]->name, name) != 0)
			break;

		size += parent[i]->nchild;
	}

	++size;
	res = calloc(size, sizeof(*res));
	size = 0;

	for (int i = 0; parent[i]; ++i) {
		if (name != NULL && strcmp(parent[i]->name, name) != 0)
			break;

		for (int j = 0; j < parent[i]->nchild; ++j)
			res[size++] = &parent[i]->child[j];
	}

	qsort(res, size, sizeof(*res), dirpcmp);
	res[size++] = NULL;
	return res;
}
