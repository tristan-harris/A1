#include "config.h"

#include "a1.h"
#include "input.h"
#include "modes.h"
#include "output.h"
#include "util.h"
#include "welcome_logo.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void editor_refresh_screen(void) {

    // +1 for padding between line number and text
    editor_state.num_col_width = num_digits(editor_state.num_rows) + 1;

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
            new_y = editor_state.cursor_y - scroll_amount;
            if (new_y < 0) { new_y = 0; }
            editor_set_cursor_y(new_y);
        }
        break;

    case DIR_DOWN:
        if (editor_state.cursor_y < editor_state.num_rows - 1) {
            new_y = editor_state.cursor_y + scroll_amount;
            if (new_y >= editor_state.num_rows) {
                new_y = editor_state.num_rows - 1;
            }
            editor_set_cursor_y(new_y);
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

void editor_draw_rows(AppendBuffer *ab) {
    char num_col_buf[15];

    bool block_cursor =
        editor_state.mode == &normal_mode || editor_state.mode == &find_mode;

    int block_cursor_y;

    if (editor_state.mode == &normal_mode) {
        block_cursor_y = editor_state.cursor_y;
    } else if (editor_state.mode == &find_mode) {
        block_cursor_y =
            editor_state.find_state.matches[editor_state.find_state.match_index]
                .row;
    }

    for (int y = 0; y < editor_state.screen_rows; y++) {
        int filerow = y + editor_state.row_scroll_offset;

        // if past text buffer, fill lines below with '~'
        if (filerow >= editor_state.num_rows) {
            ab_append(ab, "~", 1);
        } else {
            // line number
            ab_append(ab, "\x1b[2m", 4); // dim
            int num_len = snprintf(num_col_buf, sizeof(num_col_buf), "%*d ",
                                   editor_state.num_col_width - 1, filerow + 1);
            ab_append(ab, num_col_buf, num_len);
            ab_append(ab, "\x1b[m", 3); // un-dim (reset formatting)

            int line_len = editor_state.rows[filerow].render_size -
                           editor_state.col_scroll_offset;

            if (line_len >
                editor_state.screen_cols - editor_state.num_col_width) {
                line_len =
                    editor_state.screen_cols - editor_state.num_col_width;
            }

            if (line_len < 0) { line_len = 0; }

            // draw block cursor by inverting cell at cursor
            if (block_cursor && filerow == block_cursor_y) {
                if (line_len == 0) {
                    // if blank line still draw block character
                    ab_append(ab, "\x1b[7m \x1b[m", 8);
                } else {
                    ab_append(ab,
                              &editor_state.rows[filerow]
                                   .render[editor_state.col_scroll_offset],
                              editor_state.render_x -
                                  editor_state.col_scroll_offset);

                    ab_append(ab, "\x1b[7m", 4); // invert colors

                    ab_append(ab,
                              &editor_state.rows[filerow]
                                   .render[editor_state.render_x],
                              1);

                    ab_append(ab, "\x1b[m", 3); // reset formatting

                    ab_append(ab,
                              &editor_state.rows[filerow]
                                   .render[editor_state.render_x + 1],
                              line_len - editor_state.render_x +
                                  editor_state.col_scroll_offset - 1);
                }
            } else {
                ab_append(ab,
                          &editor_state.rows[filerow]
                               .render[editor_state.col_scroll_offset],
                          line_len);
            }
        }

        ab_append(ab, "\x1b[K", 3); // erase from active position to end of line
        ab_append(ab, "\r\n", 2);
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
        editor_state.filename ? editor_state.filename : "[Unnamed]");

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

    // row/col positions
    editor_add_to_status_bar_buffer(
        right_status, sizeof(right_status), &right_len, &right_render_len,
        "%d/%d, %d/%d", editor_state.cursor_y + 1, editor_state.num_rows,
        editor_state.cursor_x + 1,
        editor_state.rows[editor_state.cursor_y].size);

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
    if (editor_state.screen_cols < (int)WELCOME_LOGO_COLS + 4) { return; }
    if (editor_state.screen_rows < (int)WELCOME_LOGO_ROWS + 4) { return; }

    write(STDOUT_FILENO, "\x1b[?25l", 6); // hide cursor

    int draw_x = (editor_state.screen_cols / 2) - (WELCOME_LOGO_COLS / 2);
    int draw_y = (editor_state.screen_rows / 2) - (WELCOME_LOGO_ROWS / 2);
    int y_modifier = 0;

    // draw a1 welcome logo
    while (y_modifier < (int)WELCOME_LOGO_ROWS) {
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

    write(STDOUT_FILENO, "\x1b[?25h", 6); // show cursor
    write(STDOUT_FILENO, "\x1b[1;1H", 6); // move cursor to top left
}

void editor_set_status_message(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(editor_state.status_msg, sizeof(editor_state.status_msg), fmt,
              ap);
    va_end(ap);
    editor_state.status_msg_time = time(NULL);
}
