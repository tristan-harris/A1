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

void find_mode_entry(void *data);
void find_mode_input(int input);
void find_mode_exit(void);
FindMatch *find_matches(const char *string, int *count,
                        char *(*search_fn)(const char *, const char *));
