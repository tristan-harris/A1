#include "config.h"

#include "a1.h"
#include "argument_parse.h"
#include "file_io.h"
#include "input.h"
#include "manual.h"
#include "operations.h"
#include "output.h"
#include "terminal.h"

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
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

    // default arguments
    editor_state.arguments.clean = false;
    editor_state.arguments.config_file_path = NULL;
    editor_state.arguments.manual = false;
    editor_state.arguments.file_path = NULL;
}

void handle_window_change(int sig) {
    (void)sig; // unused

    update_window_size();
    editor_refresh_screen();
}

void apply_config_file(void) {
    char *path = editor_state.arguments.config_file_path;

    // if path specified in argument
    if (path != NULL) {
        if (file_exists(path)) {
            run_config_file(path);
        } else {
            editor_set_status_message(MSG_WARNING,
                                      "Cannot read config file at '%s'", path);
        }
    } else {
        path = get_default_config_file_path();
        if (path) {
            if (file_exists(path)) { run_config_file(path); }
            free(path);
        } else {
            editor_set_status_message(
                MSG_ERROR,
                "Could not get default config file path, is $HOME set?");
        }
    }
}

int main(int argc, char *argv[]) {
    signal(SIGWINCH, handle_window_change); // respond to window change signal

    init_editor();

    parse_arguments(&editor_state.arguments, argc, argv);

    // if manual specified, print it and exit
    if (editor_state.arguments.manual) {
        for (int i = 0; i < manual_text_len; i++) {
            puts(manual_text[i]);
        }
        exit(EXIT_SUCCESS);
    }

    if (editor_state.arguments.file_path != NULL) {
        if (!file_exists(editor_state.arguments.file_path)) {
            printf("Could not open file at '%s'\n",
                   editor_state.arguments.file_path);
            exit(1);
        }
    }

    // apply configuration
    if (!editor_state.arguments.clean) { apply_config_file(); }

    // terminal
    enable_raw_mode();
    update_window_size();

    // file io
    clear_log();

    transition_mode(&normal_mode, NULL); // start editor in normal mode

    if (editor_state.arguments.file_path != NULL) {
        open_text_file(editor_state.arguments.file_path);
    } else {
        insert_row(0, "", 0);          // empty new file
        editor_state.modified = false; // override insert_row() setting modified

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
