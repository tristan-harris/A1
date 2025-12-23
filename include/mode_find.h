#pragma once

#include <stdbool.h>

typedef struct {
    char *string;
    bool case_insensitive;
} FindModeData;

typedef struct {
    int col;
    int row;
} FindMatch;

typedef struct {
    char *string;
    FindMatch *matches;
    int matches_count;
    int match_index;
} EditorFindState;

void mode_find_entry(void *data);
void mode_find_input(int input);
void mode_find_exit(void);
