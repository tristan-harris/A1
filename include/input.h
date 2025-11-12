#pragma once

#include "a1.h"

int editor_read_key(void);
char *editor_prompt(const char *prompt);
void editor_set_cursor_x(int x);
void editor_set_cursor_y(int x);
void editor_move_cursor(EditorDirection dir);
int editor_jump_non_whitespace(EditorRow *row);
void editor_process_keypress(void);
