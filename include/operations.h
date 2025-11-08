#pragma once

#include "a1.h"

void editor_update_row(EditorRow *row);
void editor_insert_row(int row_idx, const char *string, size_t len);
void editor_free_row(const EditorRow *row);
void editor_del_row(int row_idx);
void editor_row_insert_char(EditorRow *row, int col_idx, int c);
void editor_row_append_string(EditorRow *row, const char *string, size_t len);
void editor_row_del_char(EditorRow *row, int col_idx);
void editor_row_invert_letter(EditorRow *row, int col_idx);
void editor_insert_char(int c);
void editor_insert_newline(void);
void editor_del_char(void);
