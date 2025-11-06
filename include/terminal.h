#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdio.h>

void die(const char *s);
void quit(void);
void disable_raw_mode(void);
void enable_raw_mode(void);
int editor_read_key(void);
int get_cursor_position(int *rows, int *cols);
int get_window_size(int *rows, int *cols);
void get_scroll_percentage(char *buf, size_t size);

#endif
