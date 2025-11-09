#include "config.h"

#include "input.h"
#include "operations.h"
#include "terminal.h"
#include <unistd.h>

void insert_mode_entry(void *data) {
    if (data != NULL) {
        die("insert_mode_entry");
    }
    write(STDOUT_FILENO, "\x1b[?25h", 6); // show cursor
}

void insert_mode_input(int input) {
    switch (input) {
    case '\r':
        editor_insert_newline();
        break;

    case HOME_KEY:
        editor_set_cursor_x(0);
        break;

    case END_KEY:
        editor_set_cursor_x(editor_state.rows[editor_state.cursor_y].size);
        break;

    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
        if (input == DEL_KEY) {
            editor_move_cursor(DIR_RIGHT);
        }
        editor_del_char();
        break;

    case PAGE_UP:
    case PAGE_DOWN: {
        if (input == PAGE_UP) {
            editor_state.cursor_y = editor_state.row_offset;
        } else if (input == PAGE_DOWN) {
            editor_state.cursor_y =
                editor_state.row_offset + editor_state.screen_rows - 1;
            if (editor_state.cursor_y > editor_state.num_rows)
                editor_state.cursor_y = editor_state.num_rows;
        }

        int times = editor_state.screen_rows;
        while (times--)
            editor_move_cursor(input == PAGE_UP ? DIR_UP : DIR_DOWN);
    } break;

    case ARROW_UP:
        editor_move_cursor(DIR_UP);
        break;
    case ARROW_DOWN:
        editor_move_cursor(DIR_DOWN);
        break;
    case ARROW_LEFT:
        editor_move_cursor(DIR_LEFT);
        break;
    case ARROW_RIGHT:
        editor_move_cursor(DIR_RIGHT);
        break;

    case CTRL_KEY('c'):
    case CTRL_KEY('l'):
    case ESCAPE:
        transition_mode(&normal_mode, NULL);
        break;

    default:
        editor_insert_char(input);
        break;
    }
}

void insert_mode_exit(void) {}
