#pragma once

#include "a1.h"

int editor_read_key(void);
void editor_set_cursor_x(int x);
void editor_set_cursor_y(int x);
void editor_move_new_position(EditorRow *row,
                              void move_fn(EditorRow *, int *, int *));
void editor_process_keypress(void);
