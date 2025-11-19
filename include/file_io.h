#pragma once

#include "a1.h"
#include <stdbool.h>

// main

// files can be regular, directories, sockets etc
bool file_exists(const char *file_path);

// checks whether file both exists and is a regular file
bool regular_file_exists(const char *file_path);

void get_file_permissions(const char *file_path,
                          EditorFilePermissions *file_permissions);
void open_text_file(const char *file_path);

// pass in NULL for file_path to save to same location
void save_text_buffer(const char *file_path);

// config
char *get_default_config_file_path(void);
void run_config_file(char *file_path);

// logging
void log_message(const char *fmt, ...);
void clear_log(void);
