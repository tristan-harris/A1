#pragma once

#include "mode_command.h"
#include "mode_find.h"
#include "modes.h"
#include "syntaxes.h"
#include <stdbool.h>
#include <termios.h>
#include <time.h>

#define A1_VERSION "1.0.0"
#define A1_CONFIG_DIR "a1"
#define A1_CONFIG_FILE "a1rc" // rc = run commands
#define A1_LOG_FILE "a1.log"

typedef enum {
    TAB = 9,    // horizontal tab (\t)
    ENTER = 13, // carriage return (\r)
    ESCAPE = 27,
    SPACE = 32,
    BACKSPACE = 127, // del

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
    int index;                // position in editor state rows array
    int size;                 // size of row (excluding null character)
    int render_size;          // size of rendered row
    char *chars;              // row content
    char *render;             // row content rendered to screen (needed for \t)
    unsigned char *highlight; // contains EditorHighlight data mapped to render
    bool hl_open_comment;     // if line is part of multi-line comment
} EditorRow;

typedef struct {
    bool auto_indent; // automatically indent new line based on line above
    bool case_insensitive_search; // determines default config for find mode
    bool line_numbers;            // whether to show line numbers
    bool tab_character; // whether to insert \t character when TAB key pressed
    int tab_stop;       // indentation amount
} EditorOptions;

typedef struct {
    bool clean;             // do not apply config file (a1rc)
    char *config_file_path; // apply config file at path
    bool manual;            // whether to simply print the manual and exit
    char *file_path;        // the file to edit
} EditorArguments;

typedef struct {
    bool can_read;
    bool can_write;
} EditorFilePermissions;

typedef struct {
    int cursor_x; // actual cursor x position
    int cursor_y; // actual cursor y position
    int target_x; // intended x position of cursor based on previous line(s)
    int render_x; // rendered position of cursor
    int row_scroll_offset; // offset of rows displayed (vertical scroll)
    int col_scroll_offset; // offset of columns display (horizontal scroll)
    int num_col_width;     // how many cells across the line numbers take
    int screen_rows;       // number of rows available in the emulator window
    int screen_cols;       // number of columns available in the emulator window
    int num_rows;          // number of rows that make up the text buffer
    EditorRow *rows;       // dynamic array of rows
    const EditorMode *mode;           // normal, insert, command etc.
    EditorCommandState command_state; // to store command mode variables
    EditorFindState find_state;       // to store find mode variables
    bool modified;   // whether file has been changed since last write
    char *file_path; // e.g. "../files/main.c"
    char *file_name; // e.g. "main.c"
    char status_msg[80];
    time_t status_msg_time;
    struct termios original_termios;
    EditorOptions options;
    EditorArguments arguments;
    EditorFilePermissions file_permissions;
    const EditorSyntax *syntax; // the syntax highlighting info for the current file
} EditorState;

extern EditorState editor_state;
