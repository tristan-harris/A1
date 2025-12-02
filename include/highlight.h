#pragma once

#include "a1.h"

void editor_update_syntax_highlight(EditorRow *row);
char *editor_syntax_to_sequence(EditorHighlight highlight);

// sets editor syntax to appropriate struct based on file extension
void editor_set_syntax_highlight(char *file_name);

void editor_update_syntax_highlight_all(void);

void editor_apply_find_mode_highlights(void);
