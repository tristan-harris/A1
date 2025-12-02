#include "config.h"

#include "a1.h"
#include "highlight.h"
#include "mode_find.h"
#include "modes.h"
#include "output.h"
#include "terminal.h"
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

void find_mode_entry(void *data) {
    if (data == NULL) { die("find_mode_entry recieved NULL"); }

    write(STDOUT_FILENO, "\x1b[?25l", 6); // hide cursor

    EditorFindState *fs = &editor_state.find_state;

    FindModeData *mode_data = (FindModeData *)data;
    fs->string = mode_data->string;

    int matches_count;
    char *(*search_fn)(const char *, const char *) =
        mode_data->case_insensitive ? strcasestr : strstr;

    FindMatch *matches = find_matches(fs->string, &matches_count, search_fn);

    if (matches == NULL) {
        editor_set_status_message(MSG_INFO, "No match found");
        transition_mode(&normal_mode, NULL);
        return;
    }

    fs->matches = matches;
    fs->matches_count = matches_count;
    fs->match_index = -1;

    // find first match to the right of cursor position
    for (int i = 0; i < matches_count; i++) {
        if (fs->matches[i].row == editor_state.cursor_y &&
            fs->matches[i].col >= editor_state.cursor_x) {
            fs->match_index = i;
            break;
        } else if (fs->matches[i].row > editor_state.cursor_y) {
            fs->match_index = i;
            break;
        }
    }

    // if haven't found match past cursor position, loop around to first
    if (fs->match_index == -1) { fs->match_index = 0; }

    editor_apply_find_mode_highlights();
}

void find_mode_input(int input) {
    EditorFindState *fs = &editor_state.find_state;

    switch (input) {
    // exit find mode returning cursor to original position
    case 'q':
    case ESCAPE:
        transition_mode(&normal_mode, NULL);
        break;

    // exit escape mode moving cursor to new position
    case ENTER: {
        editor_state.cursor_x = fs->matches[fs->match_index].col;
        editor_state.cursor_y = fs->matches[fs->match_index].row;
        transition_mode(&normal_mode, NULL);
        break;
    }

    // (vertically) centre view
    case 'c': {
        int buffer_rows = editor_state.screen_rows - 2;
        if (editor_state.num_rows > buffer_rows) {
            editor_state.row_scroll_offset =
                MAX(editor_state.find_state
                            .matches[editor_state.find_state.match_index]
                            .row -
                        (buffer_rows / 2),
                    0);
        }
        break;
    }

    // jump to next match
    case 'n':
        fs->match_index++;
        if (fs->match_index == fs->matches_count) { fs->match_index = 0; }
        break;

    // jump to previous match
    case 'p':
    case 'N':
        fs->match_index--;
        if (fs->match_index == -1) { fs->match_index = fs->matches_count - 1; }
        break;
    }
}

void find_mode_exit(void) {
    if (editor_state.find_state.string != NULL) {
        free(editor_state.find_state.string);
        editor_state.find_state.string = NULL;
    }

    if (editor_state.find_state.matches != NULL) {
        free(editor_state.find_state.matches);
        editor_state.find_state.matches = NULL;
    }

    editor_state.find_state.matches_count = -1;
    editor_state.find_state.match_index = -1;

    editor_update_syntax_highlight_all();
}

FindMatch *find_matches(const char *string, int *count,
                        char *(*search_fn)(const char *, const char *)) {

    int capacity = 2; // initial memory allocation for 2 FindMatch structs
    int matches_count = 0;

    FindMatch *matches = malloc(capacity * sizeof(FindMatch));

    for (int row = 0; row < editor_state.num_rows; row++) {
        int col = 0;

        while (true) {
            char *match = search_fn(&editor_state.rows[row].chars[col], string);

            if (match == NULL) { break; }

            col = match - editor_state.rows[row].chars;

            matches[matches_count].col = col;
            matches[matches_count].row = row;

            // so as to not match the same string
            col++;

            // expand matches array when required
            matches_count++;
            if (matches_count == capacity) {
                capacity *= 2;
                matches = realloc(matches, capacity * sizeof(FindMatch));
            }
        }
    }

    if (matches_count == 0) {
        free(matches);
        *count = 0;
        return NULL;
    } else {
        *count = matches_count;
        return matches;
    }
}
