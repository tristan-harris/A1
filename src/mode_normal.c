#include "config.h"

#include "a1.h"
#include "file_io.h"
#include "input.h"
#include "modes.h"
#include "operations.h"
#include "output.h"
#include "terminal.h"
#include "util.h"
#include <sys/param.h>
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

    // helpful message like in Neovim
    case CTRL_KEY('c'):
        editor_set_status_message(
            "Press 'q' to quit after saving with 's', or 'Q' "
            "to quit without saving.");
        break;

    // scroll down/up half page
    case CTRL_KEY('d'):
        editor_page_scroll(DIR_DOWN, true);
        break;
    case CTRL_KEY('u'):
        editor_page_scroll(DIR_UP, true);
        break;

    // scroll down/up full page
    case CTRL_KEY('f'):
        editor_page_scroll(DIR_DOWN, false);
        break;
    case CTRL_KEY('b'):
        editor_page_scroll(DIR_UP, false);
        break;

    // enter command mode
    case ' ':
        transition_mode(&command_mode, NULL);
        break;

    // jump to beginning of line
    case '0':
        editor_set_cursor_x(0);
        break;

    // jump to first non-whitespace character from beginning of line
    case '^':
        editor_jump_non_whitespace(row);
        break;

    // jump to end of line
    case '$':
        editor_set_cursor_x(row->size - 1);
        break;

    // enter insert mode to the right
    case 'a':
        transition_mode(&insert_mode, NULL);
        editor_move_cursor(DIR_RIGHT);
        break;
    // enter insert mode at end of line
    case 'A':
        transition_mode(&insert_mode, NULL);
        editor_set_cursor_x(row->size);
        break;

    // (vertically) centre view
    case 'c': {
        int buffer_rows = editor_state.screen_rows - 2;
        if (editor_state.num_rows > buffer_rows) {
            editor_state.row_scroll_offset =
                MAX(editor_state.cursor_y - (buffer_rows / 2), 0);
        }
        break;
    }

    // delete line
    case 'd':
        if (editor_state.num_rows > 1) {
            editor_del_row(editor_state.cursor_y);
            editor_move_cursor(DIR_UP);
        } else {
            // TODO: clear row if only row
        }
        break;

    // enter command mode with 'find ' prompt
    case 'f': {
        CommandModeData data = {.prompt = "find "};
        transition_mode(&command_mode, &data);
        break;
    }

    // jump to first line
    case 'g':
        editor_set_cursor_y(0);
        break;

    // jump to last line
    case 'G':
        editor_set_cursor_y(editor_state.num_rows - 1);
        break;

    // jump to beginning of visible line
    case 'H':
        editor_set_cursor_x(editor_state.col_scroll_offset);
        break;
    // jump to end of visible line
    case 'L': {
        int new_x = row->render_size - 1;
        if (new_x > editor_state.screen_cols - editor_state.num_col_width) {
            new_x = editor_state.screen_cols - editor_state.num_col_width - 1 +
                    editor_state.col_scroll_offset;
        }
        new_x = editor_row_rx_to_cx(row, new_x);
        editor_set_cursor_x(new_x);
        break;
    }

    // enter insert mode to the left
    case 'i':
        transition_mode(&insert_mode, NULL);
        break;
    // enter insert mode at beginning of line
    case 'I':
        editor_set_cursor_x(0);
        transition_mode(&insert_mode, NULL);
        break;

    // basic movement
    case 'h':
    case ARROW_LEFT:
        editor_move_cursor(DIR_LEFT);
        break;
    case 'j':
    case ARROW_DOWN:
        editor_move_cursor(DIR_DOWN);
        break;
    case 'k':
    case ARROW_UP:
        editor_move_cursor(DIR_UP);
        break;
    case 'l':
    case ARROW_RIGHT:
        editor_move_cursor(DIR_RIGHT);
        break;

    // enter command with 'goto ' prompt
    case 'n': {
        CommandModeData data = {.prompt = "goto "};
        transition_mode(&command_mode, &data);
        break;
    }

    // quit unmodified file
    case 'q':
        if (editor_state.modified) {
            editor_set_status_message("File has unsaved changes.");
        } else {
            quit();
        }
        break;
    // force quit
    case 'Q':
        quit();
        break;

    // save
    case 's':
        editor_save();
        break;

    // move to visible top/middle/bottom
    case 'T': // top
        editor_set_cursor_y(editor_state.row_scroll_offset);
        break;
    case 'M': // middle
        if (editor_state.row_scroll_offset + editor_state.screen_rows / 2 <
            editor_state.num_rows) {
            editor_set_cursor_y(editor_state.row_scroll_offset +
                                editor_state.screen_rows / 2);
        } else {
            editor_set_cursor_y(editor_state.num_rows - 1);
        }
        break;
    case 'B': // bottom
        if (editor_state.row_scroll_offset + editor_state.screen_rows - 1 <
            editor_state.num_rows) {
            editor_set_cursor_y(editor_state.row_scroll_offset +
                                editor_state.screen_rows - 1);
        } else {
            editor_set_cursor_y(editor_state.num_rows - 1);
        }
        break;

    // delete char
    case 'x':
        editor_row_del_char(row, editor_state.cursor_x);
        if (editor_state.cursor_x == row->size) {
            editor_move_cursor(DIR_LEFT);
        }
        break;

    // invert case
    case '~':
        editor_row_invert_letter(row, editor_state.cursor_x);
        editor_move_cursor(DIR_RIGHT);
        break;
    }
}

void normal_mode_exit(void) {}
