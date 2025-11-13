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
    editor_state.status_msg[0] = '\0';   // clear status message

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

    bool case_insensitive;

    // if 'i' or 'I' not specified, used configured default
    if (count == 2) {
        case_insensitive = editor_state.options.case_insensitive_search;
    }
    // else get first letter of third word in command buffer
    else {
        switch (words[2][0]) {
        case 'i':
            case_insensitive = true;
            break;
        case 'I':
            case_insensitive = false;
            break;
        default:
            case_insensitive = editor_state.options.case_insensitive_search;
            break;
        }
    }

    FindModeData data = {.string = strdup(words[1]),
                         .case_insensitive = case_insensitive};
    transition_mode(&find_mode, &data);
}

void goto_command(char **words, int count) {
    if (count < 2 || !is_string_integer(words[1])) {
        editor_set_status_message("Line number not specified");
        transition_mode(&normal_mode, NULL);
        return;
    }

    int line_num = atoi(words[1]);
    if (line_num <= 0 || line_num > editor_state.num_rows) {
        editor_set_status_message("Invalid line number");
        transition_mode(&normal_mode, NULL);
        return;
    }

    editor_set_cursor_y(line_num - 1);
    transition_mode(&normal_mode, NULL);
}

EditorOptionType parse_option(char *command) {
    if (strcmp(command, "cid") == 0) { return OPTION_CASE_INSENSITIVE_DEFAULT; }
    if (strcmp(command, "caseinsensitivedefault") == 0) {
        return OPTION_CASE_INSENSITIVE_DEFAULT;
    }
    if (strcmp(command, "ln") == 0) { return OPTION_LINE_NUMBERS; }
    if (strcmp(command, "linenumber") == 0) { return OPTION_LINE_NUMBERS; }
    if (strcmp(command, "tc") == 0) { return OPTION_TAB_CHARACTER; }
    if (strcmp(command, "tabcharacter") == 0) { return OPTION_TAB_CHARACTER; }
    if (strcmp(command, "ts") == 0) { return OPTION_TAB_STOP; }
    if (strcmp(command, "tabstop") == 0) { return OPTION_TAB_STOP; }

    return OPTION_UNKNOWN;
}

void set_command(char **words, int count) {
    if (count < 3) {
        editor_set_status_message("Missing option or value");
        transition_mode(&normal_mode, NULL);
        return;
    }

    EditorOptionType option_type = parse_option(words[1]);
    if (option_type == OPTION_UNKNOWN) {
        editor_set_status_message("Unknown option '%s'", words[1]);
        transition_mode(&normal_mode, NULL);
        return;
    }

    bool is_valid;

    switch (option_type) {
    case OPTION_CASE_INSENSITIVE_DEFAULT: {
        bool option_value = parse_bool(words[2], &is_valid);
        if (is_valid) {
            editor_state.options.case_insensitive_search = option_value;
        }
        break;
    }
    case OPTION_LINE_NUMBERS: {
        bool option_value = parse_bool(words[2], &is_valid);
        if (is_valid) { editor_state.options.line_numbers = option_value; }
        break;
    }
    case OPTION_TAB_CHARACTER: {
        bool option_value = parse_bool(words[2], &is_valid);
        if (is_valid) { editor_state.options.tab_character = option_value; }
        break;
    }
    case OPTION_TAB_STOP: {
        int option_value = parse_int(words[2], &is_valid);
        if (is_valid) { editor_state.options.tab_stop = option_value; }
        break;
    }
    case OPTION_UNKNOWN:
        break;
    }

    if (!is_valid) {
        editor_set_status_message("Invalid value of '%s' for option", words[2],
                                  parse_option(words[1]));
    }

    transition_mode(&normal_mode, NULL);
}

EditorCommandType parse_command(char *command) {
    if (strcmp(command, "save") == 0) { return CMD_SAVE; }
    if (strcmp(command, "find") == 0) { return CMD_FIND; }
    if (strcmp(command, "goto") == 0) { return CMD_GOTO; }
    if (strcmp(command, "set") == 0) { return CMD_SET; }
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
    case CMD_SET:
        set_command(words, count);
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
