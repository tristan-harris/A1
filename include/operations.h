#pragma once

#include "a1.h"

void update_row(EditorRow *row);
void insert_row(int row_idx, const char *string, size_t len);
void free_row(const EditorRow *row);
void del_row(int row_idx);
void insert_char_in_row(EditorRow *row, int col_idx, int c);
void append_string_to_row(EditorRow *row, const char *string, size_t len);
void del_char_at_row(EditorRow *row, int col_idx);
void del_to_previous_row(int row_idx);
void del_to_end_of_row(EditorRow *row, int col_idx);
void invert_letter_at_row(EditorRow *row, int col_idx);
void clear_row(EditorRow *row);
int auto_indent(EditorRow *row);
