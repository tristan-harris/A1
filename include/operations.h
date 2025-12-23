#pragma once

#include "a1.h"

void editor_insert_row(int row_idx, const char *string, size_t len);
void editor_insert_char_in_row(EditorRow *row, int col_idx, int c);

void editor_update_row(EditorRow *row);
void editor_append_string_to_row(EditorRow *row, const char *string,
                                 size_t len);
void editor_invert_letter_at_row(EditorRow *row, int col_idx);
// remove chars without removing row itself
void editor_clear_row(EditorRow *row);
// inserts spaces or tabs to match indentation of row above
// to be applied to a new row
// returns new cursor x
int editor_auto_indent_row(EditorRow *row);

void editor_del_row(int row_idx);
// called when user backspaces at beginning of line and there is a line above to
// be added to
void editor_del_to_previous_row(int row_idx);
void editor_del_to_end_of_row(EditorRow *row, int col_idx);
void editor_del_char_at_row(EditorRow *row, int col_idx);
