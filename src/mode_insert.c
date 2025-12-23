#include "a1.h"
#include "input.h"
#include "movement.h"
#include "operations.h"
#include "output.h"
#include "terminal.h"
#include "util.h"
#include <unistd.h>

void mode_insert_entry(void *data) {
    if (data != NULL) { terminal_die("insert_mode_entry"); }
    write(STDOUT_FILENO, "\x1b[?25h", 6); // show cursor
}

void mode_insert_input(int input) {
    EditorRow *row = &editor_state.rows[editor_state.cursor_y];

    switch (input) {
    case ENTER:
        if (editor_state.cursor_x == row->size) {
            editor_insert_row(editor_state.cursor_y + 1, "", 0);

            if (editor_state.options.auto_indent) {
                int new_cx =
                    editor_auto_indent_row(&editor_state.rows[row->index + 1]);
                editor_set_cursor_y(editor_state.cursor_y + 1);
                editor_set_cursor_x(new_cx);
            } else {
                editor_set_cursor_y(editor_state.cursor_y + 1);
                editor_set_cursor_x(0);
            }
        } else {
            if (editor_state.options.auto_indent) {
                editor_insert_row(editor_state.cursor_y + 1, "", 0);

                EditorRow *new_row =
                    &editor_state.rows[editor_state.cursor_y + 1];
                int new_cx = editor_auto_indent_row(new_row);

                editor_append_string_to_row(new_row,
                                            &row->chars[editor_state.cursor_x],
                                            row->size - editor_state.cursor_x);
                editor_del_to_end_of_row(
                    &editor_state.rows[editor_state.cursor_y],
                    editor_state.cursor_x);
                editor_set_cursor_y(editor_state.cursor_y + 1);
                editor_set_cursor_x(new_cx);
            } else {
                editor_insert_row(editor_state.cursor_y + 1,
                                  &row->chars[editor_state.cursor_x],
                                  row->size - editor_state.cursor_x);
                editor_del_to_end_of_row(
                    &editor_state.rows[editor_state.cursor_y],
                    editor_state.cursor_x);
                editor_set_cursor_y(editor_state.cursor_y + 1);
                editor_set_cursor_x(0);
            }
        }
        break;

    case TAB: {
        if (editor_state.options.tab_character) {
            editor_insert_char_in_row(row, editor_state.cursor_x, input);
            editor_move_cursor(DIR_RIGHT);
            break;
        }

        // if not inserting tab character, insert spaces up to next tab stop
        do {
            editor_insert_char_in_row(row, editor_state.cursor_x, ' ');
            editor_move_cursor(DIR_RIGHT);
        } while ((editor_state.cursor_x + editor_state.options.tab_stop) %
                     editor_state.options.tab_stop !=
                 0);

        break;
    }

    case HOME_KEY:
        editor_set_cursor_x(0);
        break;

    case END_KEY:
        editor_set_cursor_x(row->size);
        break;

    case BACKSPACE:
    case CTRL_KEY('h'): {
        // if joining onto previous line
        if (editor_state.cursor_x == 0 && editor_state.cursor_y != 0) {
            EditorRow *row_above =
                &editor_state.rows[editor_state.cursor_y - 1];
            int new_cx = row_above->size;
            int new_cy = editor_state.cursor_y - 1;

            editor_del_to_previous_row(editor_state.cursor_y);

            editor_set_cursor_y(new_cy);
            editor_set_cursor_x(new_cx);
        } else {
            int count =
                editor_get_backspace_deletion_count(row, editor_state.cursor_x);
            for (int i = 0; i < count; i++) {
                editor_del_char_at_row(row, editor_state.cursor_x - 1);
                editor_move_cursor(DIR_LEFT);
            }
        }

        break;
    }

    case DEL_KEY:
        // if at end
        if (editor_state.cursor_x == row->size) {
            // if line below, add it to focused one
            if (editor_state.cursor_y < editor_state.num_rows - 1) {
                editor_del_to_previous_row(editor_state.cursor_y + 1);
            }
        } else {
            editor_del_char_at_row(row, editor_state.cursor_x);
        }
        break;

    case PAGE_UP:
        editor_page_scroll(DIR_UP, true);
        break;
    case PAGE_DOWN:
        editor_page_scroll(DIR_DOWN, true);
        break;

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
    case ESCAPE:
        mode_transition(&normal_mode, NULL);
        break;

    default:
        // only allow ASCII character input
        if (input >= SPACE && input <= '~') {
            editor_insert_char_in_row(row, editor_state.cursor_x, input);
            editor_move_cursor(DIR_RIGHT);
        }
        break;
    }
}

void mode_insert_exit(void) {}
