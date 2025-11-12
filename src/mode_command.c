#include "config.h"

#include "a1.h"
#include "file_io.h"
#include "input.h"
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

    case ENTER:
        execute_command();
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
        // prevent input if cursor at end
        if (editor_state.command_state.cursor_x + 1 >=
            (int)(sizeof(editor_state.command_state.buffer) /
                  sizeof(editor_state.command_state.buffer[0]))) {
            break;
        }

        editor_state.command_state.buffer[editor_state.command_state.cursor_x] =
            input;
        editor_state.command_state.cursor_x++;
        editor_state.command_state.buffer[editor_state.command_state.cursor_x] =
            '\0';
        break;
    }
}

void command_mode_exit(void) {
    // dprintf(STDOUT_FILENO, "\x1b[%d;%dH", editor_state.cursor_y,
    //         editor_state.cursor_x); // move cursor
}

void find_command(char **words, int count) {
    if (count < 2) {
        editor_set_status_message("Search text not specified");
        transition_mode(&normal_mode, NULL);
        return;
    }
    FindModeData data = {.string = strdup(words[1])};
    transition_mode(&find_mode, &data);
}

void goto_command(char **words, int count) {
    if (count < 2 || !is_string_integer(words[1])) {
        editor_set_status_message("Line number not specified");
        transition_mode(&normal_mode, NULL);
        return;
    }

    int line_num = atoi(words[1]);
    if (line_num <= 0 || line_num >= editor_state.num_rows) {
        editor_set_status_message("Invalid line number");
        transition_mode(&normal_mode, NULL);
        return;
    }

    editor_set_cursor_y(line_num - 1);
    transition_mode(&normal_mode, NULL);
}

EditorCommandType parse_command(char *command) {
    if (strcmp(command, "save") == 0) { return CMD_SAVE; }
    if (strcmp(command, "find") == 0) { return CMD_FIND; }
    if (strcmp(command, "goto") == 0) { return CMD_GOTO; }
    return CMD_UNKNOWN;
}

void execute_command(void) {
    int count;
    char **words = split_string(editor_state.command_state.buffer, ' ', &count);

    if (words == NULL) {
        editor_set_status_message("Invalid input.");
        transition_mode(&normal_mode, NULL);
        return;
    }

    switch (parse_command(words[0])) {
    case CMD_SAVE:
        transition_mode(&normal_mode, NULL);
        editor_save();
        break;
    case CMD_FIND:
        find_command(words, count);
        break;
    case CMD_GOTO:
        goto_command(words, count);
        break;
    default:
        editor_set_status_message("Unknown command");
        transition_mode(&normal_mode, NULL);
    }

    for (int i = 0; i < count; i++) {
        free(words[i]);
    }
    free(words);
}
