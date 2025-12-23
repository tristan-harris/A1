#include "movement.h"
#include "a1.h"
#include "input.h"
#include "util.h"

void editor_move_cursor(EditorDirection dir) {
    EditorRow *row = &editor_state.rows[editor_state.cursor_y];

    switch (dir) {
    case DIR_UP:
        if (editor_state.cursor_y != 0) {
            editor_set_cursor_y(editor_state.cursor_y - 1);
        }
        break;

    case DIR_RIGHT:
        if (editor_state.mode == &normal_mode) {
            if (editor_state.cursor_x + 1 < row->size) {
                editor_set_cursor_x(editor_state.cursor_x + 1);
            }
        } else if (editor_state.mode == &insert_mode) {
            if (editor_state.cursor_x < row->size) {
                editor_set_cursor_x(editor_state.cursor_x + 1);
            }
        }
        break;

    case DIR_DOWN:
        if (editor_state.cursor_y + 1 < editor_state.num_rows) {
            editor_set_cursor_y(editor_state.cursor_y + 1);
        }
        break;

    case DIR_LEFT:
        if (editor_state.cursor_x > 0) {
            editor_set_cursor_x(editor_state.cursor_x - 1);
        }
        break;
    }
}

void editor_get_previous_word_start(EditorRow *row, int *new_cx, int *new_cy) {
    int cx = editor_state.cursor_x;
    int cy = editor_state.cursor_y;

    if (cx == 0 || editor_get_first_non_whitespace(row) >= cx) {
        if (cy != 0) {
            cy -= 1;
            row -= 1;
            cx = MAX(0, row->size);
        } else {
            *new_cx = editor_state.cursor_x;
            *new_cy = editor_state.cursor_y;
            return;
        }
    }

    while (cx > 0) {
        cx--;
        if (!is_whitescape(row->chars[cx]) &&
            is_whitescape(row->chars[cx - 1])) {
            *new_cx = cx;
            *new_cy = cy;
            return;
        }
    }

    *new_cx = cx;
    *new_cy = cy;
}

void editor_get_next_word_start(EditorRow *row, int *new_cx, int *new_cy) {
    int cx = editor_state.cursor_x;
    int cy = editor_state.cursor_y;

    while (cx < row->size - 1) {
        cx++;
        if (!is_whitescape(row->chars[cx]) &&
            is_whitescape(row->chars[cx - 1])) {
            *new_cx = cx;
            *new_cy = cy;
            return;
        }
    }

    if (cy == editor_state.num_rows - 1) {
        *new_cx = editor_state.cursor_x;
        *new_cy = editor_state.cursor_y;
        return;
    }

    cy += 1;
    row += 1;
    cx = editor_get_first_non_whitespace(row);
    if (cx == -1) { cx = 0; }

    *new_cx = cx;
    *new_cy = cy;
}

void editor_get_next_word_end(EditorRow *row, int *new_cx, int *new_cy) {
    int cx = editor_state.cursor_x;
    int cy = editor_state.cursor_y;

    while (cx < row->size - 1) {
        cx++;
        if (!is_whitescape(row->chars[cx])) {
            if (cx == row->size - 1 || is_whitescape(row->chars[cx + 1])) {
                *new_cx = cx;
                *new_cy = cy;
                return;
            }
        }
    }

    if (cy == editor_state.num_rows - 1) {
        *new_cx = editor_state.cursor_x;
        *new_cy = editor_state.cursor_y;
        return;
    }

    cx = 0;
    cy += 1;
    row += 1;

    while (cx < row->size - 1) {
        cx++;
        if (!is_whitescape(row->chars[cx])) {
            if (cx == row->size - 1 || is_whitescape(row->chars[cx + 1])) {
                *new_cx = cx;
                *new_cy = cy;
                return;
            }
        }
    }

    *new_cx = 0;
    *new_cy = cy;
}

void editor_get_next_blank_line(EditorRow *row, int *new_cx, int *new_cy) {
    int cy = editor_state.cursor_y;

    // whether already starting on a blank line
    bool blank_segment = row->size == 0;

    while (true) {
        // if reached/at end
        if (cy == editor_state.num_rows - 1) {
            *new_cx = row->size - 1;
            *new_cy = cy;
            return;
        }

        cy++;
        row = &editor_state.rows[cy];

        // if not already in blank segment and reached blank line
        if (!blank_segment && row->size == 0) {
            *new_cx = 0;
            *new_cy = cy;
            return;
        }

        // if no longer in blank segment
        if (blank_segment && row->size != 0) { blank_segment = false; }
    }
}

void editor_get_previous_blank_line(EditorRow *row, int *new_cx, int *new_cy) {
    int cy = editor_state.cursor_y;
    bool blank_segment = row->size == 0;

    while (true) {
        if (cy == 0) {
            *new_cx = 0;
            *new_cy = cy;
            return;
        }

        cy--;
        row = &editor_state.rows[cy];

        if (!blank_segment && row->size == 0) {
            *new_cx = 0;
            *new_cy = cy;
            return;
        }

        if (blank_segment && row->size != 0) { blank_segment = false; }
    }
}
