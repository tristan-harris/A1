#pragma once

#include <stdbool.h>

typedef struct {
    char *prompt;
} CommandModeData;

typedef struct {
    char buffer[100];
    int cursor_x;
} EditorCommandState;

void mode_command_entry(void *data);
void mode_command_input(int input);
void mode_command_exit(void);
// returns whether command is valid
bool execute_command(char *command_buffer);
