#include "config.h"

#include "a1.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

        if (current_rx > rx) { return cx; }
    }

    return cx; // only needed when rx is out of range
}

// based on stock Neovim, decided by scroll offset not cursor position
void get_scroll_percentage(char *buf, size_t size) {
    if (editor_state.row_scroll_offset == 0) {
        strncpy(buf, "Top", size);
    }
    // -1 required for empty new line (~)
    else if (editor_state.num_rows - editor_state.screen_rows ==
             editor_state.row_scroll_offset) {
        strncpy(buf, "Bot", size);
    } else {
        double percentage =
            ((double)(editor_state.row_scroll_offset) /
             (editor_state.num_rows - editor_state.screen_rows)) *
            100;
        snprintf(buf, size, "%d%%", (int)percentage);
    }
}

// serialises text buffer into single string
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

void log_message(const char *fmt, ...) {
    FILE *file = fopen("a1.log", "a");

    // timestamp
    time_t now = time(NULL);
    struct tm *time = localtime(&now);
    fprintf(file, "[%02d:%02d:%02d] ", time->tm_hour, time->tm_min,
            time->tm_sec);

    va_list args;
    va_start(args, fmt);
    vfprintf(file, fmt, args);
    va_end(args);

    fputc('\n', file);
    fclose(file);
}

void clear_log() {
    FILE *file = fopen("a1.log", "w");
    fclose(file);
}

// returns array of strings separated by delimiter (no including delimiter)
// delim character be escaped with backslash
// a replacement for `strtok` from string.h
// returns NULL if no tokens are found
char **split_string(const char *string, const char delim, int *count) {
    if (*string == '\0') { return NULL; }

    const char escape_char = '\\'; // used to escape delim char

    char *copy = strdup(string);
    char *char_ptr = copy;
    int ptr_capacity = 2; // initial memory allocation for 2 char pointers
    int substrings = 0;

    // dynamic array of char pointers
    char **tokens = malloc(ptr_capacity * sizeof(char *));

    int i = 0;
    int token_len = 0;

    while (copy[i] != '\0') {
        bool at_delim = copy[i] == delim;

        // if at the delim character and it is escaped, do not treat it as
        // delim. additionally, shift everything to the right one char to the
        // left in order to remove the escape char
        if (at_delim && i != 0 && copy[i - 1] == escape_char) {
            int right_len = strlen(&copy[i]);
            memmove(&copy[i - 1], &copy[i], right_len);
            copy[i + right_len - 1] = '\0';
            at_delim = false;
        }

        // if at end
        if (copy[i + 1] == '\0' && !at_delim) {
            tokens[substrings++] = strdup(char_ptr);
        } else if (at_delim) {
            if (token_len == 0) {
                char_ptr++;
            } else {
                copy[i] = '\0';

                // strdup() copys from pointer up to (newly placed) null
                // character
                tokens[substrings++] = strdup(char_ptr);

                char_ptr = &copy[i + 1];
                token_len = 0;
            }
        } else {
            token_len++;
        }

        // expand char pointer array if more memory is required
        if (substrings >= ptr_capacity) {
            ptr_capacity *= 2;
            tokens = realloc(tokens, ptr_capacity * sizeof(char *));
        }
        i++;
    }

    free(copy);

    if (substrings == 0) {
        free(tokens);
        *count = 0;
        return NULL;
    } else {
        *count = substrings;
        return tokens;
    }
}

// returns number of digits in number
int num_digits(int num) {
    int digits = 0;
    do {
        num /= 10;
        digits++;
    } while (num != 0);

    return digits;
}

bool is_string_integer(char *string) {
  for (int i = 0; i < strlen(string); i++) {
    if (string[i] < '0' || string[i] > '9') {
      return false;
    }
  }
  return true;
}
