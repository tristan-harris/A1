#pragma once

#include "a1.h"

void editor_process_keypress(void);
void editor_set_cursor_x(int x);
void editor_set_cursor_y(int x);
// move to new position based on supplied function
void editor_move_new_position(EditorRow *row,
                              void move_fn(EditorRow *, int *, int *));
