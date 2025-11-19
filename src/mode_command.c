#include "config.h"

#include "a1.h"
#include "file_io.h"
#include "input.h"
#include "mode_command.h"
#include "modes.h"
#include "operations.h"
#include "output.h"
#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
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
        execute_command(editor_state.command_state.buffer);
        break;

    case BACKSPACE:
    case CTRL_KEY('h'):
        editor_state.command_state.buffer[editor_state.command_state.cursor_x] =
            '\0';
        editor_state.command_state.cursor_x =
            MAX(0, editor_state.command_state.cursor_x - 1);
        break;

    case TAB:
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

bool save_command(char **words, int count) {
    // if saving to current file
    if (count == 1) {
        save_text_buffer(NULL);
    }
    // if saving with a potentially new file name
    else if (count >= 2) {
        char *file_path = words[1];
        if (file_exists(file_path) && strcmp(file_path, editor_state.file_path) != 0) {
            editor_set_status_message(MSG_WARNING, "File '%s' already exists", file_path);
        } else {
            save_text_buffer(file_path);

            if (editor_state.file_path != NULL) {
                free(editor_state.file_path);
            }
            editor_state.file_path = strdup(file_path);

            if (editor_state.file_name != NULL) {
                free(editor_state.file_name);
            }
            editor_state.file_name = file_name_from_file_path(file_path);
        }
    }

    transition_mode(&normal_mode, NULL);
    return true;
}

bool find_command(char **words, int count) {
    if (count < 2) {
        editor_set_status_message(MSG_WARNING, "Search text not specified");
        transition_mode(&normal_mode, NULL);
        return false;
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

    // replace '\t' with TAB characters
    char *search_string = replace_substr_with_char(words[1], "\\t", '\t');

    // find mode takes ownership of malloced string
    FindModeData data = {.string = search_string,
                         .case_insensitive = case_insensitive};
    transition_mode(&find_mode, &data);

    return true;
}

bool goto_command(char **words, int count) {
    if (count < 2) {
        editor_set_status_message(MSG_WARNING, "Line number not specified");
        transition_mode(&normal_mode, NULL);
        return false;
    }

    bool valid_num;
    int line_num = parse_int(words[1], &valid_num);

    if (!valid_num || line_num <= 0 || line_num > editor_state.num_rows) {
        editor_set_status_message(MSG_WARNING, "Invalid line number");
        transition_mode(&normal_mode, NULL);
        return false;
    }

    editor_set_cursor_y(line_num - 1);
    transition_mode(&normal_mode, NULL);
    return true;
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

bool get_command(char **words, int count) {
    if (count < 2) {
        editor_set_status_message(MSG_WARNING, "Missing option");
        transition_mode(&normal_mode, NULL);
        return false;
    }

    bool valid_command = true;

    EditorOptionType option_type = parse_option(words[1]);

    switch (option_type) {
    case OPTION_CASE_INSENSITIVE_DEFAULT:
        editor_set_status_message(
            MSG_INFO, "%s=%s", words[1],
            bool_to_str(editor_state.options.case_insensitive_search));
        break;
    case OPTION_LINE_NUMBERS:
        editor_set_status_message(
            MSG_INFO, "%s=%s", words[1],
            bool_to_str(editor_state.options.line_numbers));
        break;
    case OPTION_TAB_CHARACTER:
        editor_set_status_message(
            MSG_INFO, "%s=%s", words[1],
            bool_to_str(editor_state.options.tab_character));
        break;
    case OPTION_TAB_STOP:
        editor_set_status_message(MSG_INFO, "%s=%d", words[1],
                                  editor_state.options.tab_stop);
        break;
    case OPTION_UNKNOWN:
        valid_command = false;
        editor_set_status_message(MSG_WARNING, "Unknown option '%s'", words[1]);
        break;
    }

    transition_mode(&normal_mode, NULL);
    return valid_command;
}

bool set_command(char **words, int count) {
    if (count < 3) {
        editor_set_status_message(MSG_WARNING, "Missing option or value");
        transition_mode(&normal_mode, NULL);
        return false;
    }

    EditorOptionType option_type = parse_option(words[1]);
    if (option_type == OPTION_UNKNOWN) {
        editor_set_status_message(MSG_WARNING, "Unknown option '%s'", words[1]);
        transition_mode(&normal_mode, NULL);
        return false;
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
        if (is_valid) {
            if (option_value < 1) {
                editor_set_status_message(MSG_WARNING,
                                          "Invalid value of '%d' for 'tabstop'",
                                          option_value);
                is_valid = false;
                break;
            }
            editor_state.options.tab_stop = option_value;
            for (int i = 0; i < editor_state.num_rows; i++) {
                update_row(&editor_state.rows[i]);
            }
        }
        break;
    }
    case OPTION_UNKNOWN:
        is_valid = false;
        break;
    }

    if (!is_valid) {
        editor_set_status_message(MSG_WARNING, "Invalid value of '%s' for '%s'",
                                  words[2], words[1]);
    }

    transition_mode(&normal_mode, NULL);
    return is_valid;
}

EditorCommandType parse_command(char *command) {
    if (strcmp(command, "save") == 0) { return CMD_SAVE; }
    if (strcmp(command, "find") == 0) { return CMD_FIND; }
    if (strcmp(command, "goto") == 0) { return CMD_GOTO; }
    if (strcmp(command, "get") == 0) { return CMD_GET; }
    if (strcmp(command, "set") == 0) { return CMD_SET; }
    return CMD_UNKNOWN;
}

bool execute_command(char *command_buffer) {
    int count;
    char **words = split_string(command_buffer, ' ', &count);

    if (words == NULL) {
        editor_set_status_message(MSG_WARNING, "Invalid input");
        transition_mode(&normal_mode, NULL);
        return false;
    }

    bool valid_command;

    switch (parse_command(words[0])) {
    case CMD_SAVE:
        valid_command = save_command(words, count);
        break;
    case CMD_FIND:
        valid_command = find_command(words, count);
        break;
    case CMD_GOTO:
        valid_command = goto_command(words, count);
        break;
    case CMD_GET:
        valid_command = get_command(words, count);
        break;
    case CMD_SET:
        valid_command = set_command(words, count);
        break;
    default:
        editor_set_status_message(MSG_WARNING, "Unknown command '%s'",
                                  words[0]);
        transition_mode(&normal_mode, NULL);
        valid_command = false;
        break;
    }

    for (int i = 0; i < count; i++) {
        free(words[i]);
    }
    free(words);

    return valid_command;
}
