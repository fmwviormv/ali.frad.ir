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
	    "\n"
	    "#include <stdint.h>\n"
	    "\n"
	    "#ifdef __cplusplus\n"
	    "extern \"C\" {\n"
	    "#endif\n"
	    "\n"
	    "enum {\n"
	    "\t" "LANG_EN = %d,\n"
	    "\t" "LANG_FA = %d,\n"
	    "\t" "LANG_COUNT = %d\n"
	    "};\n", LANG_EN, LANG_FA, LANG_COUNT);
	fprintf(f, "\n"
	    "typedef struct {\n"
	    "\t" "int" "\t\t " "type;\n"
	    "\t" "union {\n"
	    "\t\t" "const char" "\t*" "a;\n"
	    "\t\t" "int64_t" "\t\t " "i;\n"
	    "\t" "} value;\n"
	    "} RES;\n"
	    "\n"
	    "#define ARES(X)" "\t\t" "( (RES) "
	    "{ .type = 1, .value.a = (X) } )\n"
	    "#define IRES(X, Z, F)" "\t" "( (RES) "
	    "{ .type = 256 | (((Z) & 15) << 4) | \\\n"
	    "\t\t\t    " "((F) & 15), .value.i = (X) } )\n"
	    "\n"
	    "int" "\t\t " "res_lang(const char *);\n"
	    "int" "\t\t "
	    "res_format(char *, size_t, const RES *, int);\n"
	    "\n"
	    "extern ");
	header_rec(f, 0, root_res, "res");
	fprintf(f, ";\n"
	    "\n"
	    "#ifdef __cplusplus\n"
	    "}\n"
	    "#endif\n"
	    "\n"
	    "#endif /* !__RES_H__ */\n");
	fclose(f);
	return;
}

void
header_rec(FILE *f, int indent, const struct res **res,
    const char *parent_name)
{
	const char	*name = res[0]->name;
	const struct res **child = header_child(res);

	if (!*name || isdigit(*name))
		name = parent_name;

	if (!child[0])
		header_res(f, indent, name);
	else if (!isdigit(child[0]->name[0]))
		header_dir(f, indent, child, name);
	else
		header_array(f, indent, child, name);
}

void
header_res(FILE *f, int indent, const char *name)
{
	for (int i = 0; i < indent; ++i)
		putc('\t', f);

	fprintf(f, "RES %s", name);
}

void
header_dir(FILE *f, int indent, const struct res **res,
    const char *parent_name)
{
	for (int i = 0; i < indent; ++i)
		putc('\t', f);

	fprintf(f, "struct {\n");

	for (int i = 0; res[i]; ++i) {
		const char	*name = res[i]->name;

		if (i != 0 && strcmp(name, res[i-1]->name) == 0)
			continue;

		header_rec(f, indent + 1, res + i, parent_name);
		putc(';', f);
		putc('\n', f);
	}

	for (int i = 0; i < indent; ++i)
		putc('\t', f);

	fprintf(f, "} %s", parent_name);
}

void
header_array(FILE *f, int indent, const struct res **res,
    const char *parent_name)
{
	int		 size = 0;

	for (int i = 0; res[i]; ++i) {
		int		 index = 0;
		const char	*name = res[i]->name;

		if (!isdigit(name[0]))
			continue;

		for (int j = 0; name[j] && name[j] != '.'; ++j)
			index = index * 10 + (name[j] - '0');

		if (index >= size)
			size = index + 1;
	}

	header_rec(f, indent, res, parent_name);
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
