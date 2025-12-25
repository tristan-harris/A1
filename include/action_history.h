#pragma once

typedef struct EditorActionHistory EditorActionHistory;

typedef enum {
    // insert mode
    ACTION_MODIFY_LINE,
    ACTION_NEXT_LINE,     // enter
    ACTION_PREVIOUS_LINE, // backspace

    // normal mode
    ACTION_DELETE_LINE,
    ACTION_DELETE_END_LINE, // delete to the end of the line
    ACTION_DELETE_CHAR,
    ACTION_INVERT_LETTER
} EditorActionType;

EditorActionHistory *editor_action_history_create(void);
void editor_record_action(EditorActionHistory *ah, int cursor_x, int cursor_y,
                          char *data, EditorActionType type);
void editor_undo_action(EditorActionHistory *ah);
void editor_redo_action(EditorActionHistory *ah);
void editor_action_history_destroy(EditorActionHistory *ah);
