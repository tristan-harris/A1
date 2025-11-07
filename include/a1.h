#ifndef A1_H
#define A1_H

#include "../include/modes.h"

#include <stdbool.h>
#include <termios.h>
#include <time.h>

#define A1_TAB_STOP 4

// strips every bit beyond fifth
// e.g. q (113) and Q (81) become C-Q/DC1 (17)
#define CTRL_KEY(k) ((k) & 0x1f)

typedef enum {
    ESCAPE = 27,
    BACKSPACE = 127,

    // soft codes, not related to ASCII values
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
} EditorKey;

typedef enum { DIR_UP, DIR_RIGHT, DIR_LEFT, DIR_DOWN } EditorDirection;

typedef struct {
    char *text;
    int find_cursor_x;
    int find_cursor_y;
} EditorFindState;

typedef struct {
    int size;        // size of row (excluding null character)
    int render_size; // size of rendered row
    char *chars;     // row content
    char *render;    // row content rendered to screen (needed for TABs)
} EditorRow;

typedef struct {
    int cursor_x;    // actual cursor x position
    int cursor_y;    // actual cursor y position
    int target_x;    // intended x position of cursor based on previous line(s)
    int render_x;    // rendered position of cursor
    int row_offset;  // offset of rows displayed (vertical scroll)
    int col_offset;  // offset of columns display (horizontal scroll)
    int screen_rows; // number of rows available in the emulator window
    int screen_cols; // number of columns available in the emulator window
    int num_rows;    // number of rows that make up the text buffer
    EditorRow *rows; // dynamic array of rows
    const EditorMode *mode;     // normal, insert, command etc.
    EditorFindState find_state; // to store find mode variables
    bool modified; // whether file has been changed since last write
    char *filename;
    char status_msg[80];
    time_t status_msg_time;
    struct termios original_termios;
} EditorState;

extern EditorState editor_state;

#endif
