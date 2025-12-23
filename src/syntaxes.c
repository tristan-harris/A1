#include "syntaxes.h"
#include <stdlib.h>

// NULL entries act as terminators

// ===== C =====================================================================

const char *c_file_extensions[] = {".c", ".h", ".cpp", NULL};

static const char *c_keywords[] = {
    "auto",   "break",  "case",     "const",    "continue", "default",
    "do",     "else",   "enum",     "extern",   "for",      "goto",
    "if",     "inline", "register", "restrict", "return",   "sizeof",
    "static", "struct", "switch",   "typedef",  "union",    "volatile",
    "while",  NULL};

static const char *c_types[] = {"bool",  "char",   "double",   "float", "int", "long",
                          "short", "signed", "unsigned", "void",  NULL};

static EditorSyntax c_syntax = {.file_type = "c",
                                .file_match = c_file_extensions,
                                .keywords = c_keywords,
                                .types = c_types,
                                .single_line_comment_start = "//",
                                .multi_line_comment_start = "/*",
                                .multi_line_comment_end = "*/",
                                .flags = SYNTAX_HIGHLIGHT_NUMBERS |
                                         SYNTAX_HIGHLIGHT_STRINGS};

// ===== PYTHON ================================================================

const char *python_file_extensions[] = {".py", NULL};

static const char *python_keywords[] = {
    "and",    "as",       "assert",   "async", "await",  "break",
    "case",   "class",    "continue", "def",   "del",    "elif",
    "else",   "except",   "finally",  "for",   "from",   "global",
    "if",     "import",   "in",       "is",    "lambda", "match",
    "None",   "nonlocal", "not",      "or",    "pass",   "raise",
    "return", "try",      "while",    "with",  "yield",  NULL};

static const char *python_types[] = {"int",   "float",     "bool", "str",
                               "bytes", "list",      "set",  "dict",
                               "tuple", "frozenset", NULL};

static EditorSyntax python_syntax = {.file_type = "python",
                                     .file_match = python_file_extensions,
                                     .keywords = python_keywords,
                                     .types = python_types,
                                     .single_line_comment_start = "#",
                                     .multi_line_comment_start = "'''",
                                     .multi_line_comment_end = "'''",
                                     .flags = SYNTAX_HIGHLIGHT_NUMBERS |
                                              SYNTAX_HIGHLIGHT_STRINGS};

// ===== ALL ===================================================================

const EditorSyntax *syntax_db[] = {&c_syntax, &python_syntax, NULL};
