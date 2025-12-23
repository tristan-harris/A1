#pragma once

void terminal_die(const char *s);
void terminal_quit(void);
void terminal_disable_raw_mode(void);
void terminal_enable_raw_mode(void);
int terminal_get_cursor_position(int *rows, int *cols);
int terminal_get_window_size(int *rows, int *cols);
void terminal_update_window_size(void);
