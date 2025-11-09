#include "config.h"

#include "a1.h"
#include "mode_command.h"
#include "mode_find.h"
#include "mode_insert.h"
#include "mode_normal.h"
#include "modes.h"
#include <unistd.h>

const EditorMode normal_mode = {.entry_fn = normal_mode_entry,
                                .input_fn = normal_mode_input,
                                .exit_fn = normal_mode_exit,
                                .name = "NORMAL"};

const EditorMode insert_mode = {.entry_fn = insert_mode_entry,
                                .input_fn = insert_mode_input,
                                .exit_fn = insert_mode_exit,
                                .name = "INSERT"};

const EditorMode command_mode = {.entry_fn = command_mode_entry,
                                 .input_fn = command_mode_input,
                                 .exit_fn = command_mode_exit,
                                 .name = "COMMAND"};

const EditorMode find_mode = {.entry_fn = find_mode_entry,
                              .input_fn = find_mode_input,
                              .exit_fn = find_mode_exit,
                              .name = "FIND"};

void transition_mode(const EditorMode *new_mode, void *data) {
    if (editor_state.mode != NULL) { editor_state.mode->exit_fn(); }
    editor_state.mode = new_mode;
    if (editor_state.mode != NULL) { editor_state.mode->entry_fn(data); }
}
