#pragma once

#include "a1.h"

void editor_move_cursor(EditorDirection dir);
int editor_get_first_non_whitespace(EditorRow *row);
void get_previous_word_start(EditorRow *row, int *new_cx, int *new_cy);
void get_next_word_start(EditorRow *row, int *new_cx, int *new_cy);
void get_next_word_end(EditorRow *row, int *new_cx, int *new_cy);
