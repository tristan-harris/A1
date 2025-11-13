#pragma once

// main
void editor_open(const char *filename);
void editor_save(void);

// logging
void log_message(const char *fmt, ...);
void clear_log();
