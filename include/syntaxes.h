#pragma once

#define SYNTAX_HIGHLIGHT_NUMBERS (1 << 0)
#define SYNTAX_HIGHLIGHT_STRINGS (1 << 1)

typedef struct {
    const char *file_type;
    const char **file_match; // array of strings to match filename against
    const char **keywords;   // e.g. 'extern', 'const', 'typedef' in C
    const char **types;      // e.g. 'int', 'char', 'double' in C
    const char *single_line_comment_start; // e.g. '//' in C, '#' in Python
    const char *multi_line_comment_start;  // e.g. '/*' in C, "'''" in Python
    const char *multi_line_comment_end;    // e.g. '*/' in C, "'''" in Python
    const int flags; // bit field specifying what syntax elements to highlight
} EditorSyntax;

extern const char *c_file_extensions[];
extern const char *python_file_extensions[];

extern const EditorSyntax *syntax_db[];
