#pragma once

typedef struct {
    void (*entry_fn)(void *data);
    void (*input_fn)(int input);
    void (*exit_fn)(void);
    char *name;
} EditorMode;

extern const EditorMode normal_mode;
extern const EditorMode insert_mode;
extern const EditorMode command_mode;
extern const EditorMode find_mode;

void mode_transition(const EditorMode *new_mode, void *data);
