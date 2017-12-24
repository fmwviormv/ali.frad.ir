#ifndef __RES_PARSER_H__
#include "parser.h"
#endif

const char *const res_lang_format = "};\n"
    "\n"
    "int\n"
    "res_lang(const char *lang_str)\n"
    "{\n"
    "\t" "size_t" "\t\t " "len;\n"
    "\n"
    "\t" "for (len = 0; lang_str[len]; ++len) {\n"
    "\t\t" "if (lang_str[len] == '/')\n"
    "\t\t\t" "break;\n"
    "\n"
    "\t" "for (int i = 0; i < LANG_COUNT; ++i) {\n"
    "\t\t" "if (strncmp(lang_str, lang_code[i], len + 1) == 0)\n"
    "\t\t\t" "return i;\n"
    "\t" "}\n"
    "\n"
    "\t" "return -1;\n"
    "}\n";

void
res_lang(FILE *f)
{
	fprintf(f, "#include <string.h>\n"
	    "#include \"res.h\"\n"
	    "\n"
	    "const char" "\t*" "lang_code[LANG_COUNT] = {\n");
	for (int i = 0; i < LANG_COUNT; ++i)
		fprintf(f, "\t\"%s\"%s", lang_code[i],
		    i < LANG_COUNT - 1 ? ",\n" : "\n");
	fprintf(f, res_lang_format);
}
