#pragma once

typedef struct {
    char *string;
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

void find_mode_entry(void *data);
void find_mode_input(int input);
void find_mode_exit(void);
// int editor_find(void);
FindMatch *find_matches(const char *string, int *count);
