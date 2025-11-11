#pragma once

#include "a1.h"

int editor_row_cx_to_rx(const EditorRow *row, int cx);
int editor_row_rx_to_cx(const EditorRow *row, const int rx);
void get_scroll_percentage(char *buf, size_t size);
char *editor_rows_to_string(int *buflen);
char **split_string(const char *string, const char delim, int *count);
int num_digits(int num);
