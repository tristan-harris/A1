#include "config.h"

#include "a1.h"

// NULL entries act as terminators
char *c_file_extensions[] = {".c", ".h", ".cpp", NULL};
char *python_file_extensions[] = {".py", NULL};

EditorSyntax syntax_db[1] = {
  {
    .file_type = "c",
    .file_match = c_file_extensions,
    .single_line_comment_start = "//",
    .flags = HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
  },
};
