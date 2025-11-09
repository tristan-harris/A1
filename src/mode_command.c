#include "config.h"

#include "a1.h"
#include "file_io.h"
#include "mode_command.h"
#include "modes.h"
#include "output.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void command_mode_entry(void *data) {
    // write(STDOUT_FILENO, "\x1b[?25h", 6); // show cursor
    dprintf(STDOUT_FILENO, "\x1b[?25h"); // show cursor

    if (data == NULL) {
        editor_state.command_state.buffer[0] = '\0';
    } else {
        CommandModeData *mode_data = (CommandModeData *)data;
        if (mode_data->prompt == NULL) {
            editor_state.command_state.buffer[0] = '\0';
        } else {
            strcpy(editor_state.command_state.buffer, mode_data->prompt);
        }
    }

    editor_state.command_state.cursor_x =
        strlen(editor_state.command_state.buffer);
}

void command_mode_input(int input) {
    switch (input) {
    case ESCAPE:
        transition_mode(&normal_mode, NULL);
        break;

    case '\r':
        parse_command();
        break;

    case BACKSPACE:
    case CTRL_KEY('h'):
        if (editor_state.command_state.cursor_x > 0) {
            editor_state.command_state
                .buffer[editor_state.command_state.cursor_x] = '\0';
            editor_state.command_state.cursor_x--;
        }
        break;

    case CTRL_KEY('u'):
        editor_state.command_state.buffer[0] = '\0';
        editor_state.command_state.cursor_x = 0;
        break;

    default:
        editor_state.command_state.buffer[editor_state.command_state.cursor_x] =
            input;
        editor_state.command_state.cursor_x++;
        break;
    }
}

void command_mode_exit(void) {
    // dprintf(STDOUT_FILENO, "\x1b[%d;%dH", editor_state.cursor_y,
    //         editor_state.cursor_x); // move cursor
}

void parse_command(void) {
    int count;
    char **words = split_string(editor_state.command_state.buffer, ' ', &count);

    if (words == NULL) {
        editor_set_status_message("Invalid input.");
        transition_mode(&normal_mode, NULL);
        return;
    }

    if (strcmp(words[0], "save") == 0) {
        transition_mode(&normal_mode, NULL);
        editor_save();
    } else if (strcmp(words[0], "find") == 0) {
        if (count >= 2) {
            FindModeData data = {.string = strdup(words[1])};
            transition_mode(&find_mode, &data);
        } else {
            editor_set_status_message("Search text not specified");
            transition_mode(&normal_mode, NULL);
        }
    } else {
        editor_set_status_message("Unknown command");
        transition_mode(&normal_mode, NULL);
    }

    for (int i = 0; i < count; i++) {
        free(words[i]);
    }
    free(words);
}
