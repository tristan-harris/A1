#include "config.h"

#include "input.h"
#include "util.h"
#include <stdlib.h>
#include <string.h>

void find_mode_entry(void *data) {}

void find_mode_input(int input) {
    switch (input) {}
}

void editor_find(void) {
    char *query = editor_prompt("Find: %s");
    if (query == NULL) {
        return;
    }

    for (int i = 0; i < editor_state.num_rows; i++) {
        EditorRow *row = &editor_state.rows[i];
        char *match = strstr(row->render, query);
        if (match) {
            editor_set_cursor_y(i);
            editor_set_cursor_x(editor_row_rx_to_cx(
                row, match - row->render)); // pointer arithemtic

            // so that editor_render_scroll in the next update automatically
            // scrolls up to the cursor position on the next update
            editor_state.row_offset = editor_state.num_rows;

            break;
        }
    }
    free(query);
}

void find_mode_exit(void) {}
