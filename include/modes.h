#pragma once

typedef struct {
    void (*entry_fn)(void *data);
    void (*input_fn)(int input);
    void (*exit_fn)(void);
    char *name;
} EditorMode;

typedef struct {
    char *prompt;
} CommandModeData;

typedef struct {
    char *string;
} FindModeData;

void transition_mode(const EditorMode *new_mode, void *data);

extern const EditorMode normal_mode;
extern const EditorMode insert_mode;
extern const EditorMode command_mode;
extern const EditorMode find_mode;
