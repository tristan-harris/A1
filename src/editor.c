#include "../include/config.h"

#include "../include/editor.h"

#include "../include/a1.h"
#include "../include/structures.h"
#include "../include/terminal.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

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

void editor_update_row(EditorRow *row) {
    int tabs = 0;
    for (int i = 0; i < row->size; i++) {
        if (row->chars[i] == '\t') {
            tabs++;
        }
    }

    free(row->render);
    row->render = malloc(row->size + tabs * (A1_TAB_STOP - 1) + 1);

    int idx = 0;
    for (int i = 0; i < row->size; i++) {
        if (row->chars[i] == '\t') {
            row->render[idx++] = ' ';
            while (idx % A1_TAB_STOP != 0) {
                row->render[idx++] = ' ';
            }
        } else {
            row->render[idx++] = row->chars[i];
        }
    }
    row->render[idx] = '\0';
    row->render_size = idx;
}

void editor_insert_row(const int row_idx, const char *string,
                       const size_t len) {
    if (row_idx < 0 || row_idx > editor_state.num_rows) {
        return;
    }

    editor_state.rows = realloc(
        editor_state.rows, sizeof(EditorRow) * (editor_state.num_rows + 1));
    memmove(&editor_state.rows[row_idx + 1], &editor_state.rows[row_idx],
            sizeof(EditorRow) * (editor_state.num_rows - row_idx));

    editor_state.rows[row_idx].size = len;
    editor_state.rows[row_idx].chars = malloc(len + 1);
    memcpy(editor_state.rows[row_idx].chars, string, len);
    editor_state.rows[row_idx].chars[len] = '\0';

    editor_state.rows[row_idx].render_size = 0;
    editor_state.rows[row_idx].render = NULL;
    editor_update_row(&editor_state.rows[row_idx]);

    editor_state.num_rows++;
    editor_state.dirty = true;
}

void editor_free_row(const EditorRow *row) {
    free(row->render);
    free(row->chars);
}

void editor_del_row(int row_idx) {
    if (row_idx < 0 || row_idx >= editor_state.num_rows) {
        return;
    }
    editor_free_row(&editor_state.rows[row_idx]);
    memmove(&editor_state.rows[row_idx], &editor_state.rows[row_idx + 1],
            sizeof(EditorRow) * (editor_state.num_rows - row_idx - 1));
    editor_state.num_rows--;
    editor_state.dirty = true;
}

void editor_row_insert_char(EditorRow *row, int col_idx, int c) {
    if (col_idx < 0 || col_idx > row->size) {
        col_idx = row->size;
    }
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[col_idx + 1], &row->chars[col_idx],
            row->size - col_idx + 1);
    row->size++;
    row->chars[col_idx] = c;
    editor_update_row(row);
    editor_state.dirty = true;
}

void editor_row_append_string(EditorRow *row, const char *string, size_t len) {
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], string, len);
    row->size += len;
    row->chars[row->size] = '\0';
    editor_update_row(row);
    editor_state.dirty = true;
}

void editor_row_del_char(EditorRow *row, int col_idx) {
    if (col_idx < 0 || col_idx >= row->size) {
        return;
    }
    memmove(&row->chars[col_idx], &row->chars[col_idx + 1],
            row->size - col_idx);
    row->size--;
    editor_update_row(row);
    editor_state.dirty = true;
}

void editor_row_invert_letter(EditorRow *row, int col_idx) {
    char c = row->chars[col_idx];

    // if uppercase or lower case letter
    if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122)) {
        row->chars[col_idx] ^= 0x20; // flip sixth bit to toggle case
    }
    editor_update_row(row);
}

/*** editor operations ***/

void editor_insert_char(int c) {
    if (editor_state.cursor_y == editor_state.num_rows) {
        editor_insert_row(editor_state.num_rows, "", 0);
    }
    editor_row_insert_char(&editor_state.rows[editor_state.cursor_y],
                           editor_state.cursor_x, c);
    editor_state.cursor_x++;
}

void editor_insert_newline(void) {
    if (editor_state.cursor_x == 0) {
        editor_insert_row(editor_state.cursor_y, "", 0);
    } else {
        EditorRow *row = &editor_state.rows[editor_state.cursor_y];
        editor_insert_row(editor_state.cursor_y + 1,
                          &row->chars[editor_state.cursor_x],
                          row->size - editor_state.cursor_x);

        // pointer reassignment required because realloc() in
        // editor_insert_row() may move memory and invalidate the pointer
        row = &editor_state.rows[editor_state.cursor_y];

        row->size = editor_state.cursor_x;
        row->chars[row->size] = '\0';
        editor_update_row(row);
    }
    editor_state.cursor_y++;
    editor_state.cursor_x = 0;
}

void editor_del_char(void) {
    if (editor_state.cursor_y == editor_state.num_rows) {
        return;
    }
    if (editor_state.cursor_x == 0 && editor_state.cursor_y == 0) {
        return;
    }

    EditorRow *row = &editor_state.rows[editor_state.cursor_y];
    if (editor_state.cursor_x > 0) {
        editor_row_del_char(row, editor_state.cursor_x - 1);
        editor_state.cursor_x--;
    } else {
        editor_state.cursor_x =
            editor_state.rows[editor_state.cursor_y - 1].size;
        editor_row_append_string(&editor_state.rows[editor_state.cursor_y - 1],
                                 row->chars, row->size);
        editor_del_row(editor_state.cursor_y);
        editor_state.cursor_y--;
    }
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

// returns index of first non whitespace character (i.e. not tab or space)
// returns -1 if no character found
int editor_jump_to_first_non_whitespace_char(EditorRow *row) {
    int i = 0;
    while (i < row->size) {
        if (row->chars[i] != ' ' && row->chars[i] != '\t') {
            editor_set_cursor_x(i);
            return i;
        }
        i++;
    }
    return -1;
}

/*** file i/o ***/

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

void editor_open(const char *filename) {
    free(editor_state.filename);
    editor_state.filename = strdup(filename);

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        die("fopen");
    }

    char *line = NULL;
    size_t line_cap = 0;
    ssize_t line_len; // ssize_t = signed size type
    while ((line_len = getline(&line, &line_cap, fp)) != -1) {
        // strip line-ending characters
        while (line_len > 0 &&
               (line[line_len - 1] == '\n' || line[line_len - 1] == '\r')) {
            line_len--;
        }
        editor_insert_row(editor_state.num_rows, line, line_len);
    }
    free(line);
    fclose(fp);
    editor_state.dirty = false;
}

void editor_save(void) {
    if (editor_state.filename == NULL) {
        editor_state.filename = editor_prompt("Save as: %s (ESC to cancel)");
        if (editor_state.filename == NULL) {
            editor_set_status_message("Save aborted");
            return;
        }
    }

    int len;
    char *buf = editor_rows_to_string(&len);

    int fd = open(editor_state.filename, O_RDWR | O_CREAT, 0644);
    if (fd != -1) {
        // ftruncate() adds 0 padding or deletes data to set file size same as
        // len
        if (ftruncate(fd, len) != -1) {
            if (write(fd, buf, len) == len) {
                close(fd);
                free(buf);
                editor_state.dirty = false;
                editor_set_status_message("%d bytes written to disk.", len);
                return;
            }
        }
        close(fd);
    }

    free(buf);
    editor_set_status_message("Can't save! I/O error: %s", strerror(errno));
}

void editor_find(void) {
    char *query = editor_prompt("Find: %s");
    if (query == NULL) {
        return;
    }

    for (int i = 0; i < editor_state.num_rows; i++) {
        EditorRow *row = &editor_state.rows[i];
        char *match = strstr(row->render, query);
        if (match) {
            editor_set_cursor_y(i);
            editor_set_cursor_x(editor_row_rx_to_cx(
                row, match - row->render)); // pointer arithemtic

            // so that editor_render_scroll in the next update automatically
            // scrolls up to the cursor position on the next update
            editor_state.row_offset = editor_state.num_rows;

            break;
        }
    }

    free(query);
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
            if (editor_state.num_rows == 0 &&
                y == editor_state.screen_rows / 3) {
                char welcome[80];
                int welcomelen =
                    snprintf(welcome, sizeof(welcome),
                             "A1 editor -- version %s", A1_VERSION);
                if (welcomelen > editor_state.screen_cols)
                    welcomelen = editor_state.screen_cols;
                int padding = (editor_state.screen_cols - welcomelen) / 2;
                if (padding) {
                    ab_append(ab, "~", 1);
                    padding--;
                }
                while (padding--) {
                    ab_append(ab, " ", 1);
                }
                ab_append(ab, welcome, welcomelen);
            } else {
                ab_append(ab, "~", 1);
            }
        } else {
            int len = editor_state.rows[filerow].render_size -
                      editor_state.col_offset;
            if (len < 0) {
                len = 0;
            }
            if (len > editor_state.screen_cols) {
                len = editor_state.screen_cols;
            }

            // draw block cursor by inverting cell at cursor
            if (filerow == editor_state.cursor_y &&
                editor_state.mode == &normal_mode) {
                // if blank line still draw block character
                if (len == 0) {
                    ab_append(ab, "\x1b[7m \x1b[m", 8);
                } else {
                    // FIX: col_offset crash
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
                              len - editor_state.render_x - 1);
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
                 editor_state.dirty ? "[Modified]" : "");
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

void editor_refresh_screen(void) {
    editor_scroll_render_update();

    AppendBuffer ab = {.buf = NULL, .len = 0};

    if (editor_state.mode == &insert_mode) {
        ab_append(&ab, "\x1b[?25l", 6);
    } // hide cursor

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
        ab_append(&ab, "\x1b[?25h", 6);
    } // show cursor

    write(STDOUT_FILENO, ab.buf, ab.len);
    ab_free(&ab);
}

void editor_set_status_message(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(editor_state.status_msg, sizeof(editor_state.status_msg), fmt,
              ap);
    va_end(ap);
    editor_state.status_msg_time = time(NULL);
}

/*** input ***/

char *editor_prompt(const char *prompt) {
    size_t bufsize = 128;
    char *buf = malloc(bufsize);

    size_t buflen = 0;
    buf[0] = '\0';

    while (true) {
        editor_set_status_message(prompt, buf);
        editor_refresh_screen();

        int c = editor_read_key();
        if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buflen != 0) {
                buf[--buflen] = '\0';
            }
        } else if (c == ESCAPE) {
            editor_set_status_message("");
            free(buf);
            return NULL;
        } else if (c == '\r') {
            if (buflen != 0) {
                editor_set_status_message("");
                return buf;
            }
        } else if (!iscntrl(c) && c < 128) {
            if (buflen == bufsize - 1) {
                bufsize *= 2;
                buf = realloc(buf, bufsize);
            }
            buf[buflen++] = c;
            buf[buflen] = '\0';
        }
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

void editor_move_cursor(EditorDirection dir) {
    EditorRow *row = &editor_state.rows[editor_state.cursor_y];

    switch (dir) {
    case DIR_UP:
        if (editor_state.cursor_y != 0) {
            editor_set_cursor_y(editor_state.cursor_y - 1);
        }
        break;

    case DIR_RIGHT:
        if (editor_state.mode == &normal_mode) {
            if (editor_state.cursor_x + 1 < row->size) {
                editor_set_cursor_x(editor_state.cursor_x + 1);
            }
        } else if (editor_state.mode == &insert_mode) {
            if (editor_state.cursor_x < row->size) {
                editor_set_cursor_x(editor_state.cursor_x + 1);
            }
        }
        break;

    case DIR_DOWN:
        if (editor_state.cursor_y + 1 < editor_state.num_rows) {
            editor_set_cursor_y(editor_state.cursor_y + 1);
        }
        break;

    case DIR_LEFT:
        if (editor_state.cursor_x > 0) {
            editor_set_cursor_x(editor_state.cursor_x - 1);
        }
        break;
    }
}

void editor_process_keypress(void) {
    int input = editor_read_key();
    editor_state.mode->input_fn(input);
}

void init_editor(void) {
    editor_state.cursor_x = 0;
    editor_state.cursor_y = 0;
    editor_state.target_x = 0;
    editor_state.render_x = 0;
    editor_state.row_offset = 0;
    editor_state.col_offset = 0;
    editor_state.num_rows = 0;
    editor_state.rows = NULL;

    editor_state.find_state.text = NULL;
    editor_state.find_state.find_cursor_x = 0;
    editor_state.find_state.find_cursor_y = 0;

    editor_state.dirty = false;
    editor_state.mode = NULL;
    editor_state.filename = NULL;
    editor_state.status_msg[0] = '\0';
    editor_state.status_msg_time = 0;

    int result =
        get_window_size(&editor_state.screen_rows, &editor_state.screen_cols);
    if (result == -1) {
        die("getWindowSize");
    }

    editor_state.screen_rows -= 2; // accounting for status bar and message rows

    transition_mode(&normal_mode, NULL); // start editor in normal mode
}
