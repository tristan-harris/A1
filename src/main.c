#include "config.h"

#include "a1.h"
#include "file_io.h"
#include "input.h"
#include "operations.h"
#include "output.h"
#include "terminal.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

EditorState editor_state;

void init_editor(void) {
    editor_state.cursor_x = 0;
    editor_state.cursor_y = 0;
    editor_state.target_x = 0;
    editor_state.render_x = 0;
    editor_state.row_scroll_offset = 0;
    editor_state.col_scroll_offset = 0;
    editor_state.num_rows = 0;
    editor_state.rows = NULL;

    editor_state.find_state.string = NULL;
    editor_state.find_state.cursor_x = 0;
    editor_state.find_state.cursor_y = 0;

    editor_state.modified = false;
    editor_state.mode = NULL;
    editor_state.filename = NULL;
    editor_state.status_msg[0] = '\0';
    editor_state.status_msg_time = 0;

    int result =
        get_window_size(&editor_state.screen_rows, &editor_state.screen_cols);
    if (result == -1) { die("getWindowSize"); }

    editor_state.screen_rows -= 2; // accounting for status bar and message rows

    transition_mode(&normal_mode, NULL); // start editor in normal mode
}

int main(int argc, char *argv[]) {
    enable_raw_mode();
    init_editor();

    if (argc >= 2) {
        editor_open(argv[1]);
    } else {
        editor_insert_row(0, "", 0);   // empty new file
        editor_state.modified = false; // override insert row setting modified
        editor_refresh_screen();
        editor_draw_welcome_text();
        editor_process_keypress();
    }

    while (true) {
        editor_refresh_screen();
        editor_process_keypress();
    }

    return EXIT_SUCCESS;
}
