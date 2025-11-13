#include "config.h"

#include "a1.h"
#include "file_io.h"
#include "input.h"
#include "operations.h"
#include "output.h"
#include "terminal.h"

#include <fcntl.h>
#include <signal.h>
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
    editor_state.num_col_width = 0;
    editor_state.num_rows = 0;
    editor_state.rows = NULL;

    editor_state.find_state.string = NULL;
    editor_state.find_state.matches = NULL;
    editor_state.find_state.matches_count = -1;
    editor_state.find_state.match_index = -1;

    editor_state.modified = false;
    editor_state.mode = NULL;
    editor_state.filename = NULL;
    editor_state.status_msg[0] = '\0';
    editor_state.status_msg_time = 0;

    // default configuration
    editor_state.options.case_insensitive_search = true;
    editor_state.options.line_numbers = true;
    editor_state.options.tab_character = false;
    editor_state.options.tab_stop = 4;
}

void handle_window_change(int sig) {
    (void)sig; // unused

    update_window_size();
    editor_refresh_screen();
}

int main(int argc, char *argv[]) {
    signal(SIGWINCH, handle_window_change); // respond to window change signal

    init_editor();

    // terminal
    enable_raw_mode();
    update_window_size();

    // file io
    clear_log();

    transition_mode(&normal_mode, NULL); // start editor in normal mode

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
