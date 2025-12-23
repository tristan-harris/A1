#pragma once

#include "a1.h"

typedef enum {
    HL_NORMAL,
    HL_NUMBER,
    HL_STRING,
    HL_KEYWORD,
    HL_TYPE,
    HL_SL_COMMENT, // single-line comment
    HL_ML_COMMENT, // multi-line comment
    HL_MATCH
} EditorHighlight;

void editor_update_syntax_highlight(EditorRow *row);
void editor_update_syntax_highlight_all(void);
void editor_apply_find_mode_highlights(void);
char *editor_syntax_to_sequence(EditorHighlight highlight);
// sets editor syntax to appropriate struct based on file extension
void editor_set_syntax(char *file_name);
