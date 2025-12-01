#include "config.h"

#include "a1.h"

// NULL entries act as terminators

char *c_file_extensions[] = {".c", ".h", ".cpp", NULL};

char *c_keywords[] = {"auto",     "break",   "case",   "const",    "continue",
                      "default",  "do",      "else",   "enum",     "extern",
                      "for",      "goto",    "if",     "inline",   "register",
                      "restrict", "return",  "sizeof", "static",   "struct",
                      "switch",   "typedef", "union",  "volatile", "while",
                      NULL};

char *c_types[] = {"bool",  "char",   "double",   "float", "int", "long",
                   "short", "signed", "unsigned", "void",  NULL};

char *python_file_extensions[] = {".py", NULL};

EditorSyntax syntax_db[1] = {
    {.file_type = "c",
     .file_match = c_file_extensions,
     .keywords = c_keywords,
     .types = c_types,
     .single_line_comment_start = "//",
     .multi_line_comment_start = "/*",
     .multi_line_comment_end = "*/",
     .flags = HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS},
};
