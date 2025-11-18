#include "a1.h"
#include "config.h"

#include <stddef.h>

#include "manual.h"

const char *manual_text[] = {
    "=== A1 (v" A1_VERSION ") ===",
    "A vim-based terminal text editor.\n",
    "Normal mode:",
    "{/}    -> Jump to previous/next line",
    "0      -> Jump to beginning of line",
    "^      -> Jump to first non-whitespace character in line",
    "$      -> Jump to end of line",
};

const int manual_text_len = ARRAY_LEN(manual_text);
