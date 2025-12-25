#define _GNU_SOURCE

#include "action_history.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    int cursor_x;
    int cursor_y;
    char *data;
    EditorActionType type;
} EditorAction;

struct EditorActionHistory {
    int capacity;
    int count;
    EditorAction **actions;
};

EditorActionHistory *editor_action_history_create(void) {
    EditorActionHistory *ah = malloc(sizeof *ah);
    *ah = (EditorActionHistory){.capacity = 0, .count = 0, .actions = NULL};
    return ah;
}

void editor_record_action(EditorActionHistory *ah, int cursor_x, int cursor_y,
                          char *data, EditorActionType type) {
    if (ah->count >= ah->capacity) {
        ah->capacity = ah->capacity ? ah->capacity * 2 : 1;
        ah->actions =
            realloc(ah->actions, ah->capacity * sizeof(EditorAction *));
    }
    EditorAction *action = malloc(sizeof(*action));
    *action = (EditorAction){.cursor_x = cursor_x,
                             .cursor_y = cursor_y,
                             .data = strdup(data),
                             .type = type};

    ah->actions[ah->count] = action;
    ah->count++;
}

void editor_undo_action(EditorActionHistory *ah) {}

void editor_redo_action(EditorActionHistory *ah) {}

void editor_action_history_destroy(EditorActionHistory *ah) {
    if (ah->actions) {
        for (int i = 0; i < ah->count; i++) {
            EditorAction *action = ah->actions[i];
            free(action->data);
        }
        free(ah->actions);
    }
}
