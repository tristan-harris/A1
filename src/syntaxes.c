#include "config.h"

#include "a1.h"

// NULL entries act as terminators

// ===== C =====================================================================

char *c_file_extensions[] = {".c", ".h", ".cpp", NULL};

char *c_keywords[] = {"auto",     "break",   "case",   "const",    "continue",
                      "default",  "do",      "else",   "enum",     "extern",
                      "for",      "goto",    "if",     "inline",   "register",
                      "restrict", "return",  "sizeof", "static",   "struct",
                      "switch",   "typedef", "union",  "volatile", "while",
                      NULL};

char *c_types[] = {"bool",  "char",   "double",   "float", "int", "long",
                   "short", "signed", "unsigned", "void",  NULL};

EditorSyntax c_syntax = {.file_type = "c",
                         .file_match = c_file_extensions,
                         .keywords = c_keywords,
                         .types = c_types,
                         .single_line_comment_start = "//",
                         .multi_line_comment_start = "/*",
                         .multi_line_comment_end = "*/",
                         .flags = HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS};

// ===== Python ================================================================

char *python_file_extensions[] = {".py", NULL};

char *python_keywords[] = {
    "and",    "as",       "assert",   "async", "await",  "break",
    "case",   "class",    "continue", "def",   "del",    "elif",
    "else",   "except",   "finally",  "for",   "from",   "global",
    "if",     "import",   "in",       "is",    "lambda", "match",
    "None",   "nonlocal", "not",      "or",    "pass",   "raise",
    "return", "try",      "while",    "with",  "yield",  NULL};

char *python_types[] = {"int", "float", "bool",  "str",       "bytes", "list",
                        "set", "dict",  "tuple", "frozenset", NULL};

EditorSyntax python_syntax = {.file_type = "python",
                              .file_match = python_file_extensions,
                              .keywords = python_keywords,
                              .types = python_types,
                              .single_line_comment_start = "#",
                              .multi_line_comment_start = "'''",
                              .multi_line_comment_end = "'''",
                              .flags =
                                  HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS};

EditorSyntax *syntax_db[] = {&c_syntax, &python_syntax, NULL};
