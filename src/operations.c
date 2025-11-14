#include "config.h"

#include "a1.h"
#include <stdlib.h>
#include <string.h>

void editor_update_row(EditorRow *row) {
    int tab_stop = editor_state.options.tab_stop;
    int tabs = 0;

    for (int i = 0; i < row->size; i++) {
        if (row->chars[i] == TAB) { tabs++; }
    }

    free(row->render);
    row->render = malloc(row->size + (tabs * (tab_stop - 1)) + 1);

    int idx = 0;
    for (int i = 0; i < row->size; i++) {
        if (row->chars[i] == TAB) {
            row->render[idx++] = ' ';
            while (idx % tab_stop != 0) {
                row->render[idx++] = ' ';
            }
        } else {
            row->render[idx++] = row->chars[i];
        }
    }
    row->render[idx] = '\0';
    row->render_size = idx;
}

void editor_insert_row(int row_idx, const char *string, size_t len) {
    if (row_idx < 0 || row_idx > editor_state.num_rows) { return; }

    editor_state.rows = realloc(
        editor_state.rows, sizeof(EditorRow) * (editor_state.num_rows + 1));

    // shift every following row
    memmove(&editor_state.rows[row_idx + 1], &editor_state.rows[row_idx],
            sizeof(EditorRow) * (editor_state.num_rows - row_idx));

    editor_state.rows[row_idx].size = len;
    editor_state.rows[row_idx].chars = malloc(len + 1);
    memcpy(editor_state.rows[row_idx].chars, string, len);
    editor_state.rows[row_idx].chars[len] = '\0';

    editor_state.rows[row_idx].render_size = 0;
    editor_state.rows[row_idx].render = NULL;
    editor_update_row(&editor_state.rows[row_idx]);

    editor_state.num_rows++;
    editor_state.modified = true;
}

void editor_free_row(const EditorRow *row) {
    free(row->render);
    free(row->chars);
}

void editor_del_row(int row_idx) {
    if (row_idx < 0 || row_idx >= editor_state.num_rows) { return; }
    editor_free_row(&editor_state.rows[row_idx]);
    memmove(&editor_state.rows[row_idx], &editor_state.rows[row_idx + 1],
            sizeof(EditorRow) * (editor_state.num_rows - row_idx - 1));
    editor_state.num_rows--;
    editor_state.modified = true;
}

void editor_row_insert_char(EditorRow *row, int col_idx, int c) {
    if (col_idx < 0 || col_idx > row->size) { col_idx = row->size; }
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[col_idx + 1], &row->chars[col_idx],
            row->size - col_idx + 1);
    row->size++;
    row->chars[col_idx] = c;
    editor_update_row(row);
    editor_state.modified = true;
}

void editor_row_append_string(EditorRow *row, const char *string, size_t len) {
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], string, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editor_update_row(row);
    editor_state.modified = true;
}

void editor_row_del_char(EditorRow *row, int col_idx) {
    if (col_idx < 0 || col_idx >= row->size) { return; }
    memmove(&row->chars[col_idx], &row->chars[col_idx + 1],
            row->size - col_idx);
    row->size--;
    editor_update_row(row);
    editor_state.modified = true;
}

void editor_row_invert_letter(EditorRow *row, int col_idx) {
    char c = row->chars[col_idx];

    // if uppercase or lower case letter
    if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122)) {
        row->chars[col_idx] ^= 0x20; // flip sixth bit to toggle case
    }
    editor_update_row(row);
}

/*** editor operations ***/

void editor_insert_char(int c) {
    if (editor_state.cursor_y == editor_state.num_rows) {
        editor_insert_row(editor_state.num_rows, "", 0);
    }
    editor_row_insert_char(&editor_state.rows[editor_state.cursor_y],
                           editor_state.cursor_x, c);
    editor_state.cursor_x++;
}

void editor_insert_newline(void) {
    EditorRow *row = &editor_state.rows[editor_state.cursor_y];
    editor_insert_row(editor_state.cursor_y + 1,
                      &row->chars[editor_state.cursor_x],
                      row->size - editor_state.cursor_x);

    // pointer reassignment required because realloc() in
    // editor_insert_row() may move memory and invalidate the pointer
    row = &editor_state.rows[editor_state.cursor_y];

    row->size = editor_state.cursor_x;
    row->chars[row->size] = '\0';
    editor_update_row(row);

    editor_state.cursor_y++;
    editor_state.cursor_x = 0;
}

void editor_del_char(void) {
    if (editor_state.cursor_y == editor_state.num_rows) { return; }
    if (editor_state.cursor_x == 0 && editor_state.cursor_y == 0) { return; }

    EditorRow *row = &editor_state.rows[editor_state.cursor_y];
    if (editor_state.cursor_x > 0) {
        editor_row_del_char(row, editor_state.cursor_x - 1);
        editor_state.cursor_x--;
    } else {
        editor_state.cursor_x =
            editor_state.rows[editor_state.cursor_y - 1].size;
        editor_row_append_string(&editor_state.rows[editor_state.cursor_y - 1],
                                 row->chars, row->size);
        editor_del_row(editor_state.cursor_y);
        editor_state.cursor_y--;
    }
}
