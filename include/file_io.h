#pragma once

#include "a1.h"
#include <stdbool.h>

// files can be regular, directories, sockets etc
bool file_exists(const char *file_path);
// checks whether file both exists and is a regular file
bool regular_file_exists(const char *file_path);
void get_file_permissions(const char *file_path,
                          EditorFilePermissions *file_permissions);
void editor_open_text_file(const char *file_path);
// pass in NULL for file_path to save to same location
void editor_save_text_buffer(const char *file_path);
// checks if $XDG_CONFIG_HOME is set before using $HOME
// heap-allocated, caller frees returned string
char *editor_get_default_config_file_path(void);
// if file_path is NULL, tries default path
void editor_run_config_file(char *file_path);
void editor_log_message(const char *fmt, ...);
void editor_clear_log(void);
