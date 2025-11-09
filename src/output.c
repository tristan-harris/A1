#include "config.h"

#include "input.h"
#include "output.h"
#include "util.h"
#include "welcome_logo.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void editor_refresh_screen(void) {
    editor_scroll_render_update();

    AppendBuffer ab = {.buf = NULL, .len = 0};

    if (editor_state.mode == &insert_mode) {
        ab_append(&ab, "\x1b[?25l", 6); // hide cursor
    }

    ab_append(&ab, "\x1b[H", 3); // move cursor to top-left

    editor_draw_rows(&ab);
    editor_draw_status_bar(&ab);
    editor_draw_message_bar(&ab);

    char buf[32];

    // move cursor to stored positon
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH",
             (editor_state.cursor_y - editor_state.row_offset) + 1,
             (editor_state.render_x - editor_state.col_offset) + 1);

    ab_append(&ab, buf, strlen(buf));

    if (editor_state.mode == &insert_mode) {
        ab_append(&ab, "\x1b[?25h", 6); // show cursor
    }

    write(STDOUT_FILENO, ab.buf, ab.len);
    ab_free(&ab);
}

void editor_page_scroll(EditorDirection dir, bool half) {
    int scroll_amount = editor_state.screen_rows;
    if (half) {
        scroll_amount /= 2;
    }

    int new_y;

    switch (dir) {
    case DIR_UP:
        if (editor_state.cursor_y > 0) {
            new_y = editor_state.cursor_y - scroll_amount;
            if (new_y < 0) {
                new_y = 0;
            }
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
    editor_state.render_x = 0;

    if (editor_state.cursor_y < editor_state.num_rows) {
        editor_state.render_x = editor_row_cx_to_rx(
            &editor_state.rows[editor_state.cursor_y], editor_state.cursor_x);
    }
    if (editor_state.cursor_y < editor_state.row_offset) {
        editor_state.row_offset = editor_state.cursor_y;
    }
    if (editor_state.cursor_y >=
        editor_state.row_offset + editor_state.screen_rows) {
        editor_state.row_offset =
            editor_state.cursor_y - editor_state.screen_rows + 1;
    }
    if (editor_state.render_x < editor_state.col_offset) {
        editor_state.col_offset = editor_state.render_x;
    }
    if (editor_state.render_x >=
        editor_state.col_offset + editor_state.screen_cols) {
        editor_state.col_offset =
            editor_state.render_x - editor_state.screen_cols + 1;
    }
}

void editor_draw_rows(AppendBuffer *ab) {
    for (int y = 0; y < editor_state.screen_rows; y++) {
        int filerow = y + editor_state.row_offset;

        // if past text buffer, fill lines below with '~'
        if (filerow >= editor_state.num_rows) {
            ab_append(ab, "~", 1);
        } else {
            int len = editor_state.rows[filerow].render_size -
                      editor_state.col_offset;
            if (len < 0) { len = 0; }
            if (len > editor_state.screen_cols) {
                len = editor_state.screen_cols;
            }

            // draw block cursor by inverting cell at cursor
            if (filerow == editor_state.cursor_y &&
                editor_state.mode == &normal_mode) {
                if (len == 0) {
                    // if blank line still draw block character
                    ab_append(ab, "\x1b[7m \x1b[m", 8);
                } else {
                    ab_append(ab,
                              &editor_state.rows[filerow]
                                   .render[editor_state.col_offset],
                              editor_state.render_x - editor_state.col_offset);

                    ab_append(ab, "\x1b[7m", 4); // invert colors

                    ab_append(ab,
                              &editor_state.rows[filerow]
                                   .render[editor_state.render_x],
                              1);

                    ab_append(ab, "\x1b[m", 3); // reset formatting

                    ab_append(ab,
                              &editor_state.rows[filerow]
                                   .render[editor_state.render_x + 1],
                              len + editor_state.col_offset -
                                  editor_state.render_x - 1);
                }
            } else {
                ab_append(
                    ab,
                    &editor_state.rows[filerow].render[editor_state.col_offset],
                    len);
            }
        }

        ab_append(ab, "\x1b[K", 3); // erase from active position to end of line
        ab_append(ab, "\r\n", 2);
    }
}

// TODO: improve after learning syntax highlighting
void editor_draw_status_bar(AppendBuffer *ab) {
    char left_status[80], right_status[80];

    // escape sequences bold mode text
    int left_len =
        snprintf(left_status, sizeof(left_status),
                 "\x1b[1;7m%s\x1b[0;7m %.20s %s", editor_state.mode->name,
                 editor_state.filename ? editor_state.filename : "[Unnamed]",
                 editor_state.modified ? "[Modified]" : "");
    int escape_sequence_char_count = 12;

    char scroll_percent[4]; // need to take null character (\0) into account
    get_scroll_percentage(scroll_percent, sizeof(scroll_percent));

    int right_len =
        snprintf(right_status, sizeof(right_status), "%d/%d, %d/%d (%s)",
                 editor_state.cursor_y + 1, editor_state.num_rows,
                 editor_state.cursor_x + 1,
                 editor_state.rows[editor_state.cursor_y].size, scroll_percent);

    if (left_len > editor_state.screen_cols) {
        left_len = editor_state.screen_cols;
    }
    ab_append(ab, left_status, left_len);

    left_len -= escape_sequence_char_count;

    while (left_len < editor_state.screen_cols) {
        if (editor_state.screen_cols - left_len == right_len) {
            ab_append(ab, right_status, right_len);
            break;
        } else {
            ab_append(ab, " ", 1);
            left_len++;
        }
    }

    ab_append(ab, "\x1b[m", 3); // reset formatting
    ab_append(ab, "\r\n", 2);
}

void editor_draw_message_bar(AppendBuffer *ab) {
    ab_append(ab, "\x1b[K", 3); // erase from active position to end of line
    int msglen = strlen(editor_state.status_msg);
    if (msglen > editor_state.screen_cols) {
        msglen = editor_state.screen_cols;
    }
    if (msglen && time(NULL) - editor_state.status_msg_time < 5) {
        ab_append(ab, editor_state.status_msg, msglen);
    }
}

void editor_draw_welcome_text(void) {
    char buf[32];

    // +4 to account for padding
    if (editor_state.screen_cols < (int)WELCOME_LOGO_COLS + 4) {
        return;
    }
    if (editor_state.screen_rows < (int)WELCOME_LOGO_ROWS + 4) {
        return;
    }

    write(STDOUT_FILENO, "\x1b[?25l", 6); // hide cursor

    int draw_x = (editor_state.screen_cols / 2) - (WELCOME_LOGO_COLS / 2);
    int draw_y = (editor_state.screen_rows / 2) - (WELCOME_LOGO_ROWS / 2);
    int y_modifier = 0;

    // draw a1 welcome logo
    while (y_modifier < (int)WELCOME_LOGO_ROWS) {
        snprintf(buf, sizeof(buf), "\x1b[%d;%dH", draw_y + y_modifier, draw_x);
        write(STDOUT_FILENO, buf, strlen(buf));
        write(STDOUT_FILENO, welcome_logo[y_modifier], WELCOME_LOGO_COLS);
        y_modifier++;
    }

    char subtitle_buf[50];
    int len = snprintf(subtitle_buf, sizeof(subtitle_buf),
                       "The A1 Text Editor - Version %s", A1_VERSION);

    draw_x = (editor_state.screen_cols / 2) - (strlen(subtitle_buf) / 2);
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", draw_y + y_modifier + 1, draw_x);
    write(STDOUT_FILENO, buf, strlen(buf));
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
