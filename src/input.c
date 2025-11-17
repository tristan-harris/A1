#include "config.h"

#include "a1.h"
#include "terminal.h"
#include "util.h"

#include <errno.h>
#include <sys/param.h>
#include <unistd.h>

int editor_read_key(void) {
    int bytes_read;
    char c;

    // blocking
    while ((bytes_read = read(STDIN_FILENO, &c, 1)) != 1) {
        // EAGAIN = "try again error" (required for Cygwin)
        if (bytes_read == -1 && errno != EAGAIN) {
            die("editor_read_key()");
        }
    }

    if (c == ESCAPE) {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) {
            return ESCAPE;
        }
        if (read(STDIN_FILENO, &seq[1], 1) != 1) {
            return ESCAPE;
        }

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) {
                    return ESCAPE;
                }
                if (seq[2] == '~') {
                    switch (seq[1]) {
                    case '1':
                        return HOME_KEY;
                    case '3':
                        return DEL_KEY;
                    case '4':
                        return END_KEY;
                    case '5':
                        return PAGE_UP;
                    case '6':
                        return PAGE_DOWN;
                    case '7':
                        return HOME_KEY;
                    case '8':
                        return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                case 'A':
                    return ARROW_UP;
                case 'B':
                    return ARROW_DOWN;
                case 'C':
                    return ARROW_RIGHT;
                case 'D':
                    return ARROW_LEFT;
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
            case 'H':
                return HOME_KEY;
            case 'F':
                return END_KEY;
            }
        }
        return ESCAPE;
    } else {
        return c;
    }
}

void editor_set_cursor_x(int x) {
    editor_state.cursor_x = x;
    editor_state.target_x = editor_row_cx_to_rx(
        &editor_state.rows[editor_state.cursor_y], editor_state.cursor_x);
}

void editor_set_cursor_y(int y) {
    editor_state.cursor_y = y;
    EditorRow *row = &editor_state.rows[editor_state.cursor_y];
    if (editor_state.target_x > row->size - 1) {
        editor_state.cursor_x = MAX(row->size - 1, 0);
    } else {
        editor_state.cursor_x = editor_row_rx_to_cx(row, editor_state.target_x);
    }
}

// move to new position based on supplied function
void editor_move_new_position(EditorRow *row, void move_fn(EditorRow *, int *, int *)) {
    int new_cx, new_cy;
    move_fn(row, &new_cx, &new_cy);
    if (new_cx != -1 && new_cy != -1) {
        editor_set_cursor_y(new_cy);
        editor_set_cursor_x(new_cx);
    }
}

void editor_process_keypress(void) {
    int input = editor_read_key();
    editor_state.mode->input_fn(input);
}
