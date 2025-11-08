#include "config.h"

#include "a1.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// convert cursor x position to equivalent rendered cursor x position
int editor_row_cx_to_rx(const EditorRow *row, int cx) {
    int rx = 0;
    for (int i = 0; i < cx; i++) {
        if (row->chars[i] == '\t') {
            rx += (A1_TAB_STOP - 1) - (rx % A1_TAB_STOP);
        }
        rx++;
    }
    return rx;
}

// convert rendered cursor x position to equivalent cursor x position
int editor_row_rx_to_cx(const EditorRow *row, const int rx) {
    int current_rx = 0;
    int cx;

    for (cx = 0; cx < row->size; cx++) {
        if (row->chars[cx] == '\t') {
            current_rx += (A1_TAB_STOP - 1) - (current_rx % A1_TAB_STOP);
        }
        current_rx++;

        if (current_rx > rx) {
            return cx;
        }
    }

    return cx; // only needed when rx is out of range
}

// based on stock Neovim, decided by scroll offset not cursor position
void get_scroll_percentage(char *buf, size_t size) {
    if (editor_state.row_offset == 0) {
        strncpy(buf, "Top", size);
    }
    // -1 required for empty new line (~)
    else if (editor_state.num_rows - editor_state.screen_rows ==
             editor_state.row_offset) {
        strncpy(buf, "Bot", size);
    } else {
        double percentage =
            ((double)(editor_state.row_offset) /
             (editor_state.num_rows - editor_state.screen_rows)) *
            100;
        snprintf(buf, size, "%d%%", (int)percentage);
    }
}

// serialises text buffer into single string
char *editor_rows_to_string(int *buflen) {
    int total_len = 0;
    for (int i = 0; i < editor_state.num_rows; i++) {
        total_len += editor_state.rows[i].size + 1; // +1 for newline character
    }
    *buflen = total_len;

    char *buf = malloc(total_len);
    char *buf_p = buf;
    for (int i = 0; i < editor_state.num_rows; i++) {
        memcpy(buf_p, editor_state.rows[i].chars, editor_state.rows[i].size);
        buf_p += editor_state.rows[i].size;
        *buf_p = '\n';
        buf_p++;
    }

    return buf;
}
