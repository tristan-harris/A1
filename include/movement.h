#pragma once

#include "a1.h"

void editor_move_cursor(EditorDirection dir);
// equivalent of 'B' in vim
void editor_get_previous_word_start(EditorRow *row, int *new_cx, int *new_cy);
// equivalent of 'W' in vim
void editor_get_next_word_start(EditorRow *row, int *new_cx, int *new_cy);
// equivalent of 'E' in vim
void editor_get_next_word_end(EditorRow *row, int *new_cx, int *new_cy);
// equivalent of '}' in vim
void editor_get_next_blank_line(EditorRow *row, int *new_cx, int *new_cy);
// equivalent of '{' in vim
void editor_get_previous_blank_line(EditorRow *row, int *new_cx, int *new_cy);
