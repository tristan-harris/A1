#include "config.h"

#include "a1.h"
#include "input.h"
#include "operations.h"
#include "output.h"
#include "terminal.h"
#include "util.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void editor_open(const char *filename) {
    free(editor_state.filename);
    editor_state.filename = strdup(filename);

    FILE *fp = fopen(filename, "r");
    if (!fp) { die("fopen"); }

    char *line = NULL;
    size_t line_cap = 0;
    ssize_t line_len; // ssize_t = signed size type
    while ((line_len = getline(&line, &line_cap, fp)) != -1) {
        // strip line-ending characters
        while (line_len > 0 &&
               (line[line_len - 1] == '\n' || line[line_len - 1] == '\r')) {
            line_len--;
        }
        editor_insert_row(editor_state.num_rows, line, line_len);
    }
    free(line);
    fclose(fp);
    editor_state.modified = false;
}

void editor_save(void) {
    if (editor_state.filename == NULL) {
        editor_state.filename = editor_prompt("Save as: %s (ESC to cancel)");
        if (editor_state.filename == NULL) {
            editor_set_status_message("Save aborted");
            return;
        }
    }

    int len;
    char *buf = editor_rows_to_string(&len);

    int fd = open(editor_state.filename, O_RDWR | O_CREAT, 0644);
    if (fd != -1) {
        // ftruncate() adds 0 padding or deletes data to set file size same as
        // len
        if (ftruncate(fd, len) != -1) {
            if (write(fd, buf, len) == len) {
                close(fd);
                free(buf);
                editor_state.modified = false;
                editor_set_status_message("%d bytes written to disk.", len);
                return;
            }
        }
        close(fd);
    }

    free(buf);
    editor_set_status_message("Cannot save! I/O error: %s", strerror(errno));
}

void log_message(const char *fmt, ...) {
    FILE *file = fopen("a1.log", "a");

    // timestamp
    time_t now = time(NULL);
    struct tm *time = localtime(&now);
    fprintf(file, "[%02d:%02d:%02d] ", time->tm_hour, time->tm_min,
            time->tm_sec);

    va_list args;
    va_start(args, fmt);
    vfprintf(file, fmt, args);
    va_end(args);

    fputc('\n', file);
    fclose(file);
}

void clear_log() {
    FILE *file = fopen("a1.log", "w");
    fclose(file);
}
