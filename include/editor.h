#ifndef EDITOR_H
#define EDITOR_H

#include "../include/a1.h"
#include "../include/structures.h"

/*** editor row operations ***/

// convert cursor x position to equivalent rendered cursor x position
int editor_row_cx_to_rx(const EditorRow *row, int cx);

// convert rendered cursor x position to equivalent cursor x position
int editor_row_rx_to_cx(const EditorRow *row, int rx);

void editor_update_row(EditorRow *row);
void editor_insert_row(int row_idx, const char *string, size_t len);
void editor_free_row(const EditorRow *row);
void editor_del_row(int row_idx);
void editor_row_insert_char(EditorRow *row, int col_idx, int c);
void editor_row_append_string(EditorRow *row, const char *string, size_t len);
void editor_row_del_char(EditorRow *row, int col_idx);
void editor_row_invert_letter(EditorRow *row, int col_idx);

/*** editor operations ***/

void editor_insert_char(int c);
void editor_insert_newline(void);
void editor_del_char(void);
void editor_page_scroll(EditorDirection dir, bool half);
int editor_jump_to_first_non_whitespace_char(EditorRow *row);

/*** file i/o ***/

char *editor_rows_to_string(int *buflen);

void editor_open(const char *filename);

void editor_save(void);

void editor_find(void);

void editor_scroll_render_update(void);

void editor_draw_rows(AppendBuffer *ab);

void editor_draw_status_bar(AppendBuffer *ab);

void editor_draw_message_bar(AppendBuffer *ab);

void editor_refresh_screen(void);

void editor_set_status_message(const char *fmt, ...);

/*** input ***/

char *editor_prompt(const char *prompt);
void editor_set_cursor_x(int x);
void editor_set_cursor_y(int x);
void editor_move_cursor(EditorDirection dir);
void editor_process_keypress(void);
void init_editor(void);

#endif
