#pragma once

void die(const char *s);
void quit(void);
void disable_raw_mode(void);
void enable_raw_mode(void);
int get_cursor_position(int *rows, int *cols);
int get_window_size(int *rows, int *cols);
void update_window_size(void);
