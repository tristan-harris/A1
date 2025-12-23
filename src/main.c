#include "a1.h"
#include "argument_parse.h"
#include "file_io.h"
#include "input.h"
#include "manual.h"
#include "operations.h"
#include "output.h"
#include "terminal.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

EditorState editor_state;

static void editor_init(void) {
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
    editor_state.file_path = NULL;
    editor_state.file_name = NULL;
    editor_state.status_msg[0] = '\0';
    editor_state.status_msg_time = 0;

    // default configuration
    editor_state.options.auto_indent = true;
    editor_state.options.case_insensitive_search = true;
    editor_state.options.line_numbers = true;
    editor_state.options.tab_character = false;
    editor_state.options.tab_stop = 4;

    // default arguments
    editor_state.arguments.clean = false;
    editor_state.arguments.config_file_path = NULL;
    editor_state.arguments.manual = false;
    editor_state.arguments.file_path = NULL;

    // default permissions
    editor_state.file_permissions.can_read = true;
    editor_state.file_permissions.can_write = true;
}

static void handle_window_change(int sig) {
    (void)sig; // unused

    terminal_update_window_size();
    editor_refresh_screen();
}

static void apply_config_file(void) {
    char *path = editor_state.arguments.config_file_path;

    // if path specified in argument
    if (path != NULL) {
        if (regular_file_exists(path)) {
            editor_run_config_file(path);
        } else {
            editor_set_status_message(MSG_WARNING,
                                      "Cannot read config file at '%s'", path);
        }
    } else {
        path = editor_get_default_config_file_path();
        if (path) {
            if (regular_file_exists(path)) { editor_run_config_file(path); }
            free(path);
        } else {
            editor_set_status_message(
                MSG_ERROR,
                "Could not get default config file path, is $HOME set?");
        }
    }
}

static void print_manual(void) {
    for (int i = 0; i < manual_text_len; i++) {
        puts(manual_text[i]);
    }
    exit(EXIT_SUCCESS);
}

static void manage_file(char *file_path) {
    if (!regular_file_exists(file_path)) {
        if (file_exists(file_path)) {
            printf("'%s' is not a regular file\n", file_path);
        } else {
            printf("File '%s' does not exist\n", file_path);
        }
        exit(EXIT_FAILURE);
    }
    get_file_permissions(file_path, &editor_state.file_permissions);

    if (!editor_state.file_permissions.can_read) {
        printf("Cannot read file '%s'\n", file_path);
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {
    editor_init();

    parse_arguments(&editor_state.arguments, argc, argv);

    // if manual specified, print it and exit
    if (editor_state.arguments.manual) { print_manual(); }

    // if file specified for editing
    if (editor_state.arguments.file_path != NULL) {
        manage_file(editor_state.arguments.file_path);
    }

    // apply configuration
    if (!editor_state.arguments.clean) { apply_config_file(); }

    // respond to window change signal
    signal(SIGWINCH, handle_window_change);

    // terminal
    terminal_enable_raw_mode();
    terminal_update_window_size();

    mode_transition(&normal_mode, NULL); // start editor in normal mode

    if (editor_state.arguments.file_path != NULL) {
        editor_open_text_file(editor_state.arguments.file_path);
    } else {
        editor_insert_row(0, "", 0);   // empty new file
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
