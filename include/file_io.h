#pragma once

// main
void open_text_file(const char *filename);
void save_text_file(void);

// config
char *get_config_file_path(void);
void apply_config_file(void);

// logging
void log_message(const char *fmt, ...);
void clear_log();
