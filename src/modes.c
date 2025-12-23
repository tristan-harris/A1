#include "modes.h"
#include "a1.h"
#include "mode_command.h"
#include "mode_find.h"
#include "mode_insert.h"
#include "mode_normal.h"
#include <unistd.h>

const EditorMode normal_mode = {.entry_fn = mode_normal_entry,
                                .input_fn = mode_normal_input,
                                .exit_fn = mode_normal_exit,
                                .name = "NORMAL"};

const EditorMode insert_mode = {.entry_fn = mode_insert_entry,
                                .input_fn = mode_insert_input,
                                .exit_fn = mode_insert_exit,
                                .name = "INSERT"};

const EditorMode command_mode = {.entry_fn = mode_command_entry,
                                 .input_fn = mode_command_input,
                                 .exit_fn = mode_command_exit,
                                 .name = "COMMAND"};

const EditorMode find_mode = {.entry_fn = mode_find_entry,
                              .input_fn = mode_find_input,
                              .exit_fn = mode_find_exit,
                              .name = "FIND"};

void mode_transition(const EditorMode *new_mode, void *data) {
    if (editor_state.mode != NULL) { editor_state.mode->exit_fn(); }
    editor_state.mode = new_mode;
    if (editor_state.mode != NULL) { editor_state.mode->entry_fn(data); }
}
