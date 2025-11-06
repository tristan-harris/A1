#ifndef MODES_H
#define MODES_H

typedef struct {
    void (*entry_fn)(void *data);
    void (*input_fn)(int input);
    void (*exit_fn)(void);
    char *name;
} EditorMode;

void transition_mode(const EditorMode *new_mode, void *data);

void normal_mode_entry(void *data);
void normal_mode_input(int input);
void normal_mode_exit(void);

void insert_mode_entry(void *data);
void insert_mode_input(int input);
void insert_mode_exit(void);

void command_mode_entry(void *data);
void command_mode_input(int input);
void command_mode_exit(void);

void find_mode_entry(void *data);
void find_mode_input(int input);
void find_mode_exit(void);

extern const EditorMode normal_mode;
extern const EditorMode insert_mode;
extern const EditorMode command_mode;
extern const EditorMode find_mode;

#endif
