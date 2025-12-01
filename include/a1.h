#pragma once

#include "mode_command.h"
#include "mode_find.h"
#include "modes.h"
#include <stdbool.h>
#include <termios.h>
#include <time.h>

// strips every bit beyond fifth
// e.g. q (113) and Q (81) become C-Q/DC1 (17)
#define CTRL_KEY(k) ((k) & 0x1f)

#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof((arr)[0]))

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

typedef enum {
    HL_NORMAL,
    HL_NUMBER,
    HL_STRING,
    HL_KEYWORD,
    HL_TYPE,
    HL_SL_COMMENT, // single-line comment
    HL_ML_COMMENT, // multi-line comment
    HL_MATCH
} EditorHighlight;

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
    char *file_type;
    char **file_match; // array of strings to match filename against
    char **keywords;   // e.g. 'extern', 'const', 'typedef' in C
    char **types;      // e.g. 'int', 'char', 'double' in C
    char *single_line_comment_start; // e.g. '//' in C, '#' in Python
    char *multi_line_comment_start;  // e.g. '/*' in C, "'''" in Python
    char *multi_line_comment_end;    // e.g. '*/' in C, "'''" in Python
    int flags; // bit field specifying what syntax elements to highlight
} EditorSyntax;

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
    EditorSyntax *syntax; // the syntax highlighting info for the current file
} EditorState;

extern EditorState editor_state;
