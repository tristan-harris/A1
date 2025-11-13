#pragma once

typedef enum { CMD_SAVE, CMD_FIND, CMD_GOTO, CMD_UNKNOWN } EditorCommandType;

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
void execute_command(void);
