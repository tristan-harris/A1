#pragma once

#include "a1.h"

// ===== MACROS ================================================================

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

// strips every bit beyond fifth
// e.g. q (113) and Q (81) become C-Q/DC1 (17)
#define CTRL_KEY(k) ((k) & 0x1f)

// ===== EDITOR ================================================================

// convert cursor x position to equivalent rendered cursor x position
int editor_row_cx_to_rx(const EditorRow *row, int cx);
// convert rendered cursor x position to equivalent cursor x position
int editor_row_rx_to_cx(const EditorRow *row, const int rx);
// based on stock Neovim, decided by scroll offset not cursor position
void editor_get_scroll_percentage(char *buf, size_t size);
// serialises text buffer into single string
char *editor_rows_to_string(int *buf_len);
// returns number of characters to delete to the left of cursor
// will return larger number if there are multiple spaces
int editor_get_backspace_deletion_count(const EditorRow *row, int cursor_x);
// returns index of first non whitespace character (i.e. not tab or space)
// returns -1 if no character found
int editor_get_first_non_whitespace(EditorRow *row);

// ===== GENERAL ===============================================================

char *bool_to_str(const bool boolean);
// returns number of digits in number
int num_digits(int num);
// only accepts positive numbers
bool is_string_integer(const char *string);
bool is_whitescape(const char c);
bool parse_bool(const char *string, bool *valid);
int parse_int(const char *string, bool *valid);
// returns heap-allocated str
char *file_name_from_file_path(const char *file_path);
// returns array of strings separated by delimiter (not including delimiter)
// delim character can be escaped with backslash
// a replacement for `strtok` from string.h
// returns NULL if no tokens are found
char **split_string(const char *string, const char delim, int *count);
// replaces every instance of substring with a char (constructive
// implementation)
char *replace_substr_with_char(const char *str, const char *substr,
                               const char character);
