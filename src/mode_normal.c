#include "config.h"

#include "a1.h"
#include "file_io.h"
#include "input.h"
#include "modes.h"
#include "operations.h"
#include "output.h"
#include "terminal.h"
#include <unistd.h>

void normal_mode_entry(void *data) {
    if (data != NULL) { die("normal_mode_entry"); }

    write(STDOUT_FILENO, "\x1b[?25l", 6); // hide cursor

    // move back from end of line (needed for transitioning from insert mode)
    if (editor_state.rows != NULL) {
        if (editor_state.cursor_x ==
            editor_state.rows[editor_state.cursor_y].size) {
            editor_move_cursor(DIR_LEFT);
        }
    }
}

void normal_mode_input(int input) {
    EditorRow *row = &editor_state.rows[editor_state.cursor_y];

    switch (input) {
    case CTRL_KEY('c'):
        editor_set_status_message(
            "Press 'q' to quit after saving with 's', or 'Q' "
            "to quit without saving.");
        break;

    case CTRL_KEY('d'):
        editor_page_scroll(DIR_DOWN, true);
        break;
    case CTRL_KEY('u'):
        editor_page_scroll(DIR_UP, true);
        break;

    case CTRL_KEY('f'):
        editor_page_scroll(DIR_DOWN, false);
        break;
    case CTRL_KEY('b'):
        editor_page_scroll(DIR_UP, false);
        break;

    case ' ':
        transition_mode(&command_mode, NULL);
        break;

    case '0':
        editor_set_cursor_x(0);
        break;

    case '^':
        editor_jump_invert_char(row);
        break;

    case '$':
        editor_set_cursor_x(row->size - 1);
        break;

    case 'a':
        transition_mode(&insert_mode, NULL);
        editor_move_cursor(DIR_RIGHT);
        break;
    case 'A':
        transition_mode(&insert_mode, NULL);
        editor_set_cursor_x(row->size);
        break;

    case 'd':
        if (editor_state.num_rows > 1) {
            editor_del_row(editor_state.cursor_y);
            editor_move_cursor(DIR_UP);
        } else {
            // TODO: clear row if only row
        }
        break;

    case 'f': {
        CommandModeData data = {.prompt = "find "};
        transition_mode(&command_mode, &data);
        break;
    }

    case 'g':
        editor_set_cursor_y(0);
        break;

    case 'G':
        editor_set_cursor_y(editor_state.num_rows - 1);
        break;

    case 'H':
        editor_set_cursor_y(editor_state.row_offset);
        break;
    case 'M':
        if (editor_state.row_offset + editor_state.screen_rows / 2 <
            editor_state.num_rows) {
            editor_set_cursor_y(editor_state.row_offset +
                                editor_state.screen_rows / 2);
        } else {
            editor_set_cursor_y(editor_state.num_rows - 1);
        }
        break;
    case 'L':
        if (editor_state.row_offset + editor_state.screen_rows - 1 <
            editor_state.num_rows) {
            editor_set_cursor_y(editor_state.row_offset +
                                editor_state.screen_rows - 1);
        } else {
            editor_set_cursor_y(editor_state.num_rows - 1);
        }
        break;

    case 'i':
        transition_mode(&insert_mode, NULL);
        break;
    case 'I':
        editor_set_cursor_x(0);
        transition_mode(&insert_mode, NULL);
        break;

    case 'h':
        editor_move_cursor(DIR_LEFT);
        break;
    case 'j':
        editor_move_cursor(DIR_DOWN);
        break;
    case 'k':
        editor_move_cursor(DIR_UP);
        break;
    case 'l':
        editor_move_cursor(DIR_RIGHT);
        break;

    case 'q':
        if (editor_state.modified) {
            editor_set_status_message("File has unsaved changes.");
        } else {
            quit();
        }
        break;
    case 'Q':
        quit();
        break;

    case 's':
        editor_save();
        break;

    case 'x':
        editor_row_del_char(row, editor_state.cursor_x);
        if (editor_state.cursor_x == row->size) {
            editor_move_cursor(DIR_LEFT);
        }
        break;

    case '~':
        editor_row_invert_letter(row, editor_state.cursor_x);
        editor_move_cursor(DIR_RIGHT);
        break;
    }
}

void normal_mode_exit(void) {}
