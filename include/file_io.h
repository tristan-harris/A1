#pragma once

#include "a1.h"

#include <stdbool.h>

// main
bool file_exists(const char *file_path);
void get_file_permissions(const char *file_path,
                          EditorFilePermissions *file_permissions);
void open_text_file(const char *filename);
void save_text_buffer(void);

// config
char *get_default_config_file_path(void);
void run_config_file(char *file_path);

// logging
void log_message(const char *fmt, ...);
void clear_log(void);
