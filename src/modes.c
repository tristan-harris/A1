#include "../include/config.h"

#include "../include/modes.h"

#include "../include/a1.h"
#include "../include/editor.h"
#include "../include/terminal.h"

#include <unistd.h>

const EditorMode normal_mode = {.entry_fn = normal_mode_entry,
                                .input_fn = normal_mode_input,
                                .exit_fn = normal_mode_exit,
                                .name = "NORMAL "};

const EditorMode insert_mode = {.entry_fn = insert_mode_entry,
                                .input_fn = insert_mode_input,
                                .exit_fn = insert_mode_exit,
                                .name = "INSERT "};

const EditorMode command_mode = {.entry_fn = command_mode_entry,
                                 .input_fn = command_mode_input,
                                 .exit_fn = command_mode_exit,
                                 .name = "COMMAND"};

const EditorMode find_mode = {.entry_fn = find_mode_entry,
                              .input_fn = find_mode_input,
                              .exit_fn = find_mode_exit,
                              .name = "FIND   "};

void transition_mode(const EditorMode *new_mode, void *data) {
    if (editor_state.mode != NULL) {
        editor_state.mode->exit_fn();
    }
    editor_state.mode = new_mode;
    if (editor_state.mode != NULL) {
        editor_state.mode->entry_fn(data);
    }
}

void normal_mode_entry(void *data) {
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
        editor_jump_to_first_non_whitespace_char(row);
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
        editor_del_row(editor_state.cursor_y);
        break;

    case 'f':
        transition_mode(&command_mode, "Find: ");
        break;

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
        if (editor_state.dirty) {
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
        break;
    }
}

void normal_mode_exit(void) {}

void insert_mode_entry(void *data) {
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

void command_mode_entry(void *data) {}

void command_mode_input(int input) {
    switch (input) {
    case ESCAPE:
        transition_mode(&normal_mode, NULL);
        break;
    case '\r':
        transition_mode(&normal_mode, NULL);
        break;
    }
}

void command_mode_exit(void) {}

void find_mode_entry(void *data) {}

void find_mode_input(int input) {
    switch (input) {}
}

void find_mode_exit(void) {}
