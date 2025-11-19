#include "config.h"

#include "a1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ===== EDITOR =====

// convert cursor x position to equivalent rendered cursor x position
int editor_row_cx_to_rx(const EditorRow *row, int cx) {
    int rx = 0;
    int tab_stop = editor_state.options.tab_stop;

    for (int i = 0; i < cx; i++) {
        if (row->chars[i] == TAB) { rx += (tab_stop - 1) - (rx % tab_stop); }
        rx++;
    }
    return rx;
}

// convert rendered cursor x position to equivalent cursor x position
int editor_row_rx_to_cx(const EditorRow *row, const int rx) {
    int tab_stop = editor_state.options.tab_stop;

    int current_rx = 0;
    int cx;

    for (cx = 0; cx < row->size; cx++) {
        if (row->chars[cx] == TAB) {
            current_rx += (tab_stop - 1) - (current_rx % tab_stop);
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
    } else if (editor_state.num_rows - editor_state.screen_rows ==
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

// returns number of characters to delete to the left of cursor
// will return larger number if there are multiple spaces
int get_backspace_deletion_count(const EditorRow *row, int cursor_x) {
    if (cursor_x < 2) { return cursor_x; }

    int count = 0;
    int i = cursor_x - 1;

    while (true) {
        // if reached end
        if (i < 0) { break; }

        // if reached non-space character
        if (row->chars[i] != SPACE) { break; }

        // check if at tab stop increment
        if ((i + editor_state.options.tab_stop) %
                editor_state.options.tab_stop ==
            0) {
            count++;
            break;
        }

        i--;
        count++;
    }

    if (count == 0) { count = 1; }

    return count;
}

// ===== GENERAL =====

char *bool_to_str(const bool boolean) {
    return boolean ? "true" : "false";
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

bool is_string_integer(const char *string) {
    for (int i = 0; i < (int)strlen(string); i++) {
        if (string[i] < '0' || string[i] > '9') { return false; }
    }
    return true;
}

bool is_whitescape(const char c) {
    return c == SPACE || c == TAB;
}

bool parse_bool(const char *string, bool *valid) {
    *valid = true;
    if (strcmp(string, "true") == 0) { return true; }
    if (strcmp(string, "false") == 0) { return false; }
    if (strcmp(string, "yes") == 0) { return true; }
    if (strcmp(string, "no") == 0) { return false; }
    if (strcmp(string, "1") == 0) { return true; }
    if (strcmp(string, "0") == 0) { return false; }

    *valid = false;
    return false;
}

int parse_int(const char *string, bool *valid) {
    if (!is_string_integer(string)) {
        *valid = false;
        return -1;
    }

    int num = 0;
    *valid = true;

    for (int i = 0; i < (int)strlen(string); i++) {
        if (i != 0) { num *= 10; }
        num += string[i] - 48;
    }

    return num;
}

char *file_name_from_file_path(const char *file_path) {
    const char *separator = "/";
    const char *ch_ptr = file_path;
    const char *next = file_path;

    // ch_ptr will point to last occurence of separator
    // or beginning of string if there are none
    while ((next = strstr(next + 1, separator)) != NULL) {
        ch_ptr = next;
    }

    // do not include separator itself
    if (*ch_ptr == *separator) { ch_ptr++; }

    return strndup(ch_ptr, strlen(ch_ptr));
}

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

char *replace_substr_with_char(const char *str, const char *substr,
                               const char character) {
    char *new_str = strdup(str);
    size_t sub_len = strlen(substr);
    char *ch_ptr = new_str;

    while ((ch_ptr = strstr(ch_ptr, substr)) != NULL) {
        // replace
        *ch_ptr = character;

        // shift rest of string left by one char
        memmove(ch_ptr + 1, ch_ptr + sub_len, strlen(ch_ptr + sub_len) + 1);

        ch_ptr += 1;
    }

    return new_str;
}
