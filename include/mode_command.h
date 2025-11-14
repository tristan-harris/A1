#pragma once

#include <stdbool.h>

typedef enum {
    CMD_SAVE,
    CMD_FIND,
    CMD_GOTO,
    CMD_GET,
    CMD_SET,
    CMD_UNKNOWN
} EditorCommandType;

typedef enum {
    OPTION_CASE_INSENSITIVE_DEFAULT,
    OPTION_LINE_NUMBERS,
    OPTION_TAB_CHARACTER,
    OPTION_TAB_STOP,
    OPTION_UNKNOWN
} EditorOptionType;

typedef struct {
    char *prompt;
} CommandModeData;

typedef struct {
    char buffer[100];
    int cursor_x;
} EditorCommandState;

void command_mode_entry(void *data);
void command_mode_input(int input);
void command_mode_exit(void);
bool execute_command(char *command_buffer);
