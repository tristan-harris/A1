#include "config.h"

#include "a1.h"
#include "operations.h"
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

// ===== util =====

void update_row(EditorRow *row) {
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

// ===== main =====

void insert_row(int idx, const char *string, size_t len) {
    if (idx < 0 || idx > editor_state.num_rows) { return; }

    editor_state.rows = realloc(
        editor_state.rows, sizeof(EditorRow) * (editor_state.num_rows + 1));

    // shift every following row
    memmove(&editor_state.rows[idx + 1], &editor_state.rows[idx],
            sizeof(EditorRow) * (editor_state.num_rows - idx));

    editor_state.rows[idx].size = len;
    editor_state.rows[idx].chars = malloc(len + 1);
    memcpy(editor_state.rows[idx].chars, string, len);
    editor_state.rows[idx].chars[len] = '\0';

    editor_state.rows[idx].render_size = 0;
    editor_state.rows[idx].render = NULL;
    update_row(&editor_state.rows[idx]);

    editor_state.num_rows++;
    editor_state.modified = true;
}

void insert_char_in_row(EditorRow *row, int col_idx, int character) {
    if (col_idx < 0 || col_idx > row->size) { col_idx = row->size; }

    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[col_idx + 1], &row->chars[col_idx],
            row->size - col_idx + 1);
    row->size++;
    row->chars[col_idx] = character;
    update_row(row);
    editor_state.modified = true;
}

void append_string_to_row(EditorRow *row, const char *string, size_t len) {
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], string, len);
    row->size += len;
    row->chars[row->size] = '\0';
    update_row(row);
    editor_state.modified = true;
}

void invert_letter_at_row(EditorRow *row, int col_idx) {
    char c = row->chars[col_idx];

    // if uppercase or lower case letter
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        row->chars[col_idx] ^= 0x20; // flip sixth bit to toggle case
    }
    update_row(row);
}

void del_row(int row_idx) {
    if (row_idx < 0 || row_idx >= editor_state.num_rows) { return; }

    EditorRow *row = &editor_state.rows[row_idx];
    free(row->render);
    free(row->chars);

    memmove(&editor_state.rows[row_idx], &editor_state.rows[row_idx + 1],
            sizeof(EditorRow) * (editor_state.num_rows - row_idx - 1));
    editor_state.num_rows--;
    editor_state.modified = true;
}

void del_char_at_row(EditorRow *row, int col_idx) {
    if (col_idx < 0 || col_idx >= row->size) { return; }

    memmove(&row->chars[col_idx], &row->chars[col_idx + 1],
            row->size - col_idx);
    row->size--;
    update_row(row);
    editor_state.modified = true;
}

// called when user backspaces at beginning of line and there is a line above to
// be added to
void del_to_previous_row(int row_idx) {
    if (row_idx == 0) { return; }

    EditorRow *row = &editor_state.rows[row_idx];

    append_string_to_row(row - 1, row->chars, row->size);
    del_row(row_idx);
}

void del_to_end_of_row(EditorRow *row, int col_idx) {
    if (col_idx < 0 || col_idx >= row->size) { return; }

    row->chars = realloc(row->chars, col_idx + 1);
    row->size = col_idx;
    row->chars[row->size] = '\0';
    update_row(row);
    editor_state.modified = true;
}

// remove chars without removing row itself
void clear_row(EditorRow *row) {
    row->chars = realloc(row->chars, sizeof(char));
    row->size = 0;
    row->chars[0] = '\0';
    update_row(row);
    editor_state.modified = true;
}
