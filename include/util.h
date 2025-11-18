#pragma once

#include "a1.h"

// ===== EDITOR =====

int editor_row_cx_to_rx(const EditorRow *row, int cx);
int editor_row_rx_to_cx(const EditorRow *row, const int rx);
void get_scroll_percentage(char *buf, size_t size);
char *editor_rows_to_string(int *buflen);
int get_backspace_deletion_count(const EditorRow *row, int cursor_x);

// ===== GENERAL =====

char **split_string(const char *string, const char delim, int *count);

// replaces every instance of substring with a char (constructive implementation)
char *replace_substr_with_char(const char *str, const char *substr, const char character);

int num_digits(int num);
bool is_string_integer(const char *string);
bool is_whitescape(const char c);
bool parse_bool(const char *string, bool *valid);
int parse_int(const char *string, bool *valid);
char *bool_to_str(const bool boolean);
