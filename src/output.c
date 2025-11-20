#include "config.h"

#include "a1.h"
#include "highlight.h"
#include "input.h"
#include "modes.h"
#include "output.h"
#include "util.h"
#include "welcome_logo.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

void editor_refresh_screen(void) {

    if (editor_state.options.line_numbers) {
        // +1 for padding between line number and text
        editor_state.num_col_width = num_digits(editor_state.num_rows) + 1;
    } else {
        editor_state.num_col_width = 0;
    }

    editor_scroll_render_update();

    // to be added to with the contents of the screen
    AppendBuffer ab = {.buf = NULL, .len = 0};

    if (editor_state.mode == &insert_mode) {
        ab_append(&ab, "\x1b[?25l", 6); // hide cursor
    }

    ab_append(&ab, "\x1b[H", 3); // move cursor to top-left

    editor_draw_rows(&ab);
    editor_draw_status_bar(&ab);
    editor_draw_bottom_bar(&ab);

    char command_buf[32];

    // if command mode, return cursor to bottom of screen at input buffer
    // else return to text editor buffer position
    if (editor_state.mode == &command_mode) {
        snprintf(command_buf, sizeof(command_buf), "\x1b[%d;%dH",
                 editor_state.screen_cols - 1,
                 editor_state.command_state.cursor_x + 1);
    } else {
        snprintf(command_buf, sizeof(command_buf), "\x1b[%d;%dH",
                 (editor_state.cursor_y - editor_state.row_scroll_offset) + 1,
                 (editor_state.render_x - editor_state.col_scroll_offset +
                  editor_state.num_col_width) +
                     1);
    }

    ab_append(&ab, command_buf, strlen(command_buf));

    if (editor_state.mode == &insert_mode) {
        ab_append(&ab, "\x1b[?25h", 6); // show cursor
    }

    write(STDOUT_FILENO, ab.buf, ab.len);
    ab_free(&ab);
}

void editor_page_scroll(EditorDirection dir, bool half) {
    int scroll_amount = editor_state.screen_rows;
    if (half) { scroll_amount /= 2; }

    int new_y;

    switch (dir) {
    case DIR_UP:
        if (editor_state.cursor_y > 0) {
            new_y = MAX(editor_state.cursor_y - scroll_amount, 0);
            editor_set_cursor_y(new_y);

            if (new_y > editor_state.screen_rows) {
                editor_state.row_scroll_offset =
                    new_y - editor_state.screen_rows;
            } else {
                editor_state.row_scroll_offset = 0;
            }
        }
        break;

    case DIR_DOWN:
        if (editor_state.cursor_y < editor_state.num_rows - 1) {
            new_y = MIN(editor_state.cursor_y + scroll_amount,
                        editor_state.num_rows - 1);
            editor_set_cursor_y(new_y);

            if (new_y + scroll_amount < editor_state.num_rows) {
                editor_state.row_scroll_offset = MIN(
                    new_y, editor_state.num_rows - editor_state.screen_rows);
            }
        }
        break;

    default:
        break;
    }
}

void editor_scroll_render_update(void) {
    int y_position;

    // update render_x and get y_position based on mode
    if (editor_state.mode == &find_mode) {
        FindMatch *fm = &editor_state.find_state
                             .matches[editor_state.find_state.match_index];
        editor_state.render_x =
            editor_row_cx_to_rx(&editor_state.rows[fm->row], fm->col);
        y_position = fm->row;
    } else {
        editor_state.render_x = editor_row_cx_to_rx(
            &editor_state.rows[editor_state.cursor_y], editor_state.cursor_x);
        y_position = editor_state.cursor_y;
    }

    // if scrolled up
    if (y_position < editor_state.row_scroll_offset) {
        editor_state.row_scroll_offset = y_position;
    }

    // if scrolled down
    if (y_position >=
        editor_state.row_scroll_offset + editor_state.screen_rows) {
        editor_state.row_scroll_offset =
            y_position - editor_state.screen_rows + 1;
    }

    // if scrolled left
    if (editor_state.render_x < editor_state.col_scroll_offset) {
        editor_state.col_scroll_offset = editor_state.render_x;
    }

    // if scrolled right
    if (editor_state.render_x >=
        editor_state.col_scroll_offset +
            (editor_state.screen_cols - editor_state.num_col_width)) {
        editor_state.col_scroll_offset =
            editor_state.render_x -
            (editor_state.screen_cols - editor_state.num_col_width) + 1;
    }
}

// utility function for editor_draw_rows()
void editor_add_row_end(AppendBuffer *ab) {
    ab_append(ab, "\x1b[39m", 5); // default text color
    ab_append(ab, "\x1b[K", 3);   // erase from active position to end of line
    ab_append(ab, "\r\n", 2);
}

// utility function for editor_draw_rows()
bool editor_at_block_cursor(int cur_row, int cur_col) {
    bool at_row = false;
    bool at_col = false;

    if (editor_state.mode == &normal_mode) {
        at_row = cur_row == editor_state.cursor_y;
        at_col = cur_col == editor_state.render_x;
    } else if (editor_state.mode == &find_mode) {
        at_row = cur_row == editor_state.find_state
                                .matches[editor_state.find_state.match_index]
                                .row;
        at_col = cur_col == editor_state.render_x;
    }
    return at_row && at_col;
}

void editor_draw_rows(AppendBuffer *ab) {
    char esc_seq_buf[15];

    bool block_cursor =
        editor_state.mode == &normal_mode || editor_state.mode == &find_mode;

    for (int y = 0; y < editor_state.screen_rows; y++) {
        int row_index = y + editor_state.row_scroll_offset;
        EditorRow *row = &editor_state.rows[row_index];

        // if past text buffer, fill lines below with '~'
        if (row_index >= editor_state.num_rows) {
            ab_append(ab, "~", 1);
            editor_add_row_end(ab);
            continue;
        }

        // line numbers
        if (editor_state.options.line_numbers) {
            ab_append(ab, "\x1b[2m", 4); // dim

            int num_len =
                snprintf(esc_seq_buf, sizeof(esc_seq_buf), "%*d ",
                         editor_state.num_col_width - 1, row_index + 1);

            ab_append(ab, esc_seq_buf, num_len);
            ab_append(ab, "\x1b[22m", 5); // un-dim (normal intensity)
        }

        int line_len = row->render_size - editor_state.col_scroll_offset;

        // fit line to screen
        line_len = MIN(line_len,
                       editor_state.screen_cols - editor_state.num_col_width);
        line_len = MAX(line_len, 0);

        // draw block character on empty line
        if (line_len == 0) {
            if (block_cursor && editor_at_block_cursor(row_index, 0)) {
                ab_append(ab, "\x1b[7m", 4); // invert
                ab_append(ab, " ", 1);
                ab_append(ab, "\x1b[27m", 5); // not invert
                editor_add_row_end(ab);
                continue;
            }
        }

        EditorHighlight cur_hl = HL_NORMAL;

        for (int x = 0; x < line_len; x++) {
            int col_index = editor_state.col_scroll_offset + x;
            EditorHighlight next_hl = row->highlight[col_index];

            // if different highlight, add escape sequence
            if (cur_hl != next_hl) {
                int len = snprintf(esc_seq_buf, sizeof(esc_seq_buf), "\x1b[%sm",
                                   editor_syntax_to_sequence(next_hl));
                ab_append(ab, esc_seq_buf, len);
                cur_hl = next_hl;
            }

            // block cursor invert
            if (block_cursor && editor_at_block_cursor(row_index, col_index)) {
                ab_append(ab, "\x1b[7m", 4); // invert
                ab_append(ab, &row->render[col_index], 1);
                ab_append(ab, "\x1b[27m", 5); // not invert
            } else {
                ab_append(ab, &row->render[col_index], 1);
            }
        }
        editor_add_row_end(ab);
    }
}

// utility function for status bar text
// strings containing ANSI escape sequences should only contain those sequences
void editor_add_to_status_bar_buffer(char buffer[], size_t max_len,
                                     int *buffer_len, int *buffer_render_len,
                                     const char *fmt, ...) {
    if (*buffer_len >= (int)max_len) { return; }

    va_list ap;
    va_start(ap, fmt);
    int written_len =
        vsnprintf(&buffer[*buffer_len], max_len - *buffer_len, fmt, ap);
    va_end(ap);

    // if not writing escape code
    if (buffer[*buffer_len] != '\x1b') { *buffer_render_len += written_len; }
    *buffer_len += written_len;
}

void editor_draw_status_bar(AppendBuffer *ab) {
    char left_status[100], right_status[100];

    // the length of the buffers in memory
    int left_len = 0, right_len = 0;

    // the rendered length of the buffers (no escape code characters)
    int left_render_len = 0, right_render_len = 0;

    // bold & invert
    editor_add_to_status_bar_buffer(left_status, sizeof(left_status), &left_len,
                                    &left_render_len, "\x1b[1;7m");
    // mode name
    editor_add_to_status_bar_buffer(left_status, sizeof(left_status), &left_len,
                                    &left_render_len, "%s",
                                    editor_state.mode->name);
    // just invert
    editor_add_to_status_bar_buffer(left_status, sizeof(left_status), &left_len,
                                    &left_render_len, "\x1b[0;7m");
    // find string
    if (editor_state.mode == &find_mode) {
        editor_add_to_status_bar_buffer(left_status, sizeof(left_status),
                                        &left_len, &left_render_len, " ('%s')",
                                        editor_state.find_state.string);
    }

    // filename
    editor_add_to_status_bar_buffer(
        left_status, sizeof(left_status), &left_len, &left_render_len, " %s",
        editor_state.file_name ? editor_state.file_name : "[Unnamed]");

    // ready-only
    editor_add_to_status_bar_buffer(
        left_status, sizeof(left_status), &left_len, &left_render_len, "%s",
        !editor_state.file_permissions.can_write ? " [Read-Only]" : "");

    // modified
    editor_add_to_status_bar_buffer(left_status, sizeof(left_status), &left_len,
                                    &left_render_len, "%s",
                                    editor_state.modified ? " [Modified]" : "");

    // match index/number of matches
    if (editor_state.mode == &find_mode) {
        editor_add_to_status_bar_buffer(
            right_status, sizeof(right_status), &right_len, &right_render_len,
            "[%d/%d] ", editor_state.find_state.match_index + 1,
            editor_state.find_state.matches_count);
    }

    // row/col positions (find mode)
    if (editor_state.mode == &find_mode) {
        FindMatch *fm = &editor_state.find_state
                             .matches[editor_state.find_state.match_index];

        editor_add_to_status_bar_buffer(
            right_status, sizeof(right_status), &right_len, &right_render_len,
            "%d/%d, %d/%d", fm->row + 1, editor_state.num_rows, fm->col + 1,
            editor_state.rows[fm->row].size);
    }
    // row/col positions (normal mode default)
    else {
        editor_add_to_status_bar_buffer(
            right_status, sizeof(right_status), &right_len, &right_render_len,
            "%d/%d, %d/%d", editor_state.cursor_y + 1, editor_state.num_rows,
            editor_state.cursor_x + 1,
            editor_state.rows[editor_state.cursor_y].size);
    }

    // scroll percentage
    char scroll_percent[4]; // need to take null character (\0) into account
    get_scroll_percentage(scroll_percent, sizeof(scroll_percent));
    editor_add_to_status_bar_buffer(right_status, sizeof(right_status),
                                    &right_len, &right_render_len, " (%s)",
                                    scroll_percent);

    // calculate padding between left and right status sections

    if (left_render_len > editor_state.screen_cols) {
        left_render_len = editor_state.screen_cols;
    }

    ab_append(ab, left_status, left_len);

    while (left_render_len < editor_state.screen_cols) {
        if (editor_state.screen_cols - left_render_len == right_render_len) {
            ab_append(ab, right_status, right_len);
            break;
        } else {
            ab_append(ab, " ", 1);
            left_render_len++;
        }
    }

    ab_append(ab, "\x1b[m", 3); // reset formatting
    ab_append(ab, "\r\n", 2);
}

void editor_draw_bottom_bar(AppendBuffer *ab) {
    ab_append(ab, "\x1b[K", 3); // erase from active position to end of line
    if (editor_state.mode == &command_mode) {
        ab_append(ab, editor_state.command_state.buffer,
                  editor_state.command_state.cursor_x);
    }
    // draw message if not in command mode
    else {
        int msglen = strlen(editor_state.status_msg);
        if (msglen > editor_state.screen_cols) {
            msglen = editor_state.screen_cols;
        }
        if (msglen && time(NULL) - editor_state.status_msg_time < 5) {
            ab_append(ab, editor_state.status_msg, msglen);
        }
    }
}

void editor_draw_welcome_text(void) {
    // +4 to account for padding
    if (editor_state.screen_cols < WELCOME_LOGO_COLS + 4) { return; }
    if (editor_state.screen_rows < WELCOME_LOGO_ROWS + 4) { return; }

    int draw_x = (editor_state.screen_cols / 2) - (WELCOME_LOGO_COLS / 2);
    int draw_y = (editor_state.screen_rows / 2) - (WELCOME_LOGO_ROWS / 2);
    int y_modifier = 0;

    // draw a1 welcome logo
    while (y_modifier < WELCOME_LOGO_ROWS) {
        dprintf(STDOUT_FILENO, "\x1b[%d;%dH", draw_y + y_modifier, draw_x);
        write(STDOUT_FILENO, welcome_logo[y_modifier], WELCOME_LOGO_COLS);
        y_modifier++;
    }

    char subtitle_buf[50];
    int len = snprintf(subtitle_buf, sizeof(subtitle_buf),
                       "The A1 Text Editor - Version %s", A1_VERSION);

    draw_x = (editor_state.screen_cols / 2) - (strlen(subtitle_buf) / 2);
    dprintf(STDOUT_FILENO, "\x1b[%d;%dH", draw_y + y_modifier + 1, draw_x);
    write(STDOUT_FILENO, subtitle_buf, len);
}

void editor_clear_status_message(void) {
    editor_state.status_msg[0] = '\0';
}

void editor_set_status_message(StatusMessageType msg_type, const char *fmt,
                               ...) {
    int sequence_written = 0;

    switch (msg_type) {
    case MSG_INFO:
        // default text color
        break;

    case MSG_WARNING:
        // yellow
        sequence_written =
            snprintf(editor_state.status_msg, sizeof(editor_state.status_msg),
                     "\x1b[33m");
        break;

    case MSG_ERROR:
        // red
        sequence_written =
            snprintf(editor_state.status_msg, sizeof(editor_state.status_msg),
                     "\x1b[31m");
        break;
    }

    // needed to add to end of status_msg
    char *reset = "\x1b[0m";

    // the space available for the message itself, guaranteeing space for the
    // reset sequence
    int msg_len_max =
        sizeof(editor_state.status_msg) - sequence_written - strlen(reset);

    va_list ap;
    va_start(ap, fmt);
    int msg_written = vsnprintf(editor_state.status_msg + sequence_written,
                                msg_len_max, fmt, ap);
    va_end(ap);

    // if message longer than available buffer space
    // add escape code at reserved space at end
    if (msg_written > msg_len_max) {
        snprintf(editor_state.status_msg +
                     (sizeof(editor_state.status_msg) - strlen(reset) - 1),
                 sizeof(editor_state.status_msg), "%s", reset);
    }
    // else add reset sequence after main message
    else {
        snprintf(editor_state.status_msg + sequence_written + msg_written,
                 sizeof(editor_state.status_msg), "%s", reset);
    }

    editor_state.status_msg_time = time(NULL);
}
