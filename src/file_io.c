#include "config.h"

#include "a1.h"
#include "mode_command.h"
#include "operations.h"
#include "output.h"
#include "terminal.h"
#include "util.h"
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// checks whether file both exists and can be read
bool file_exists(const char *file_path) {
    return access(file_path, F_OK) == 0;
}

void get_file_permissions(const char *file_path,
                          EditorFilePermissions *file_permissions) {
    file_permissions->can_read = access(file_path, R_OK) == 0;
    file_permissions->can_write = access(file_path, W_OK) == 0;
}

void open_text_file(const char *file_path) {
    free(editor_state.file_path);
    editor_state.file_path = strdup(file_path);
    editor_state.file_name = file_name_from_file_path(editor_state.file_path);

    FILE *fp = fopen(file_path, "r");
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
        insert_row(editor_state.num_rows, line, line_len);
    }
    free(line);
    fclose(fp);
    editor_state.modified = false;
}

void save_text_buffer(void) {
    if (!editor_state.file_permissions.can_write) {
        editor_set_status_message(MSG_WARNING,
                                  "Cannot save, file is read-only.");
        return;
    }

    if (editor_state.file_path == NULL) {
        return;
        // editor_state.filename = editor_prompt("Save as: %s (ESC to cancel)");
        // if (editor_state.filename == NULL) {
        //     editor_set_status_message(MSG_INFO, "Save aborted");
        //     return;
        // }
    }

    int len;
    char *buf = editor_rows_to_string(&len);

    int fd = open(editor_state.file_path, O_RDWR | O_CREAT, 0644);
    if (fd != -1) {
        // ftruncate() adds 0 padding or deletes data to set file size same as
        // len
        if (ftruncate(fd, len) != -1) {
            if (write(fd, buf, len) == len) {
                close(fd);
                free(buf);
                editor_state.modified = false;
                editor_set_status_message(MSG_INFO, "%d bytes written to disk.",
                                          len);
                return;
            }
        }
        close(fd);
    }

    free(buf);
    editor_set_status_message(MSG_ERROR, "Cannot save! I/O error: %s",
                              strerror(errno));
}

// checks if $XDG_CONFIG_HOME is set before using $HOME
// heap-allocated, caller frees returned string
char *get_default_config_file_path(void) {
    char *xdg_cfg_dir = getenv("XDG_CONFIG_HOME");
    char *home_dir = getenv("HOME");

    char *file_path;

    if (xdg_cfg_dir) {
        asprintf(&file_path, "%s/%s/%s", xdg_cfg_dir, A1_CONFIG_DIR,
                 A1_CONFIG_FILE);
    } else if (home_dir) {
        asprintf(&file_path, "%s/.config/%s/%s", home_dir, A1_CONFIG_DIR,
                 A1_CONFIG_FILE);
    } else {
        return NULL;
    }

    return file_path;
}

// if file_path is NULL, try default path
void run_config_file(const char *file_path) {
    FILE *file = fopen(file_path, "r");

    if (!file) { die("run_config_file"); }

    char line[1000];

    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0'; // strip newline
        line[strcspn(line, "#")] = '\0';  // strip comments

        if (*line == '\0') { continue; }

        // only run set commands
        if (strncmp(line, "set", 3) == 0) {
            bool valid_command = execute_command(line);
            if (!valid_command) { break; }
        } else {
            editor_set_status_message(
                MSG_ERROR, "Invalid configuration command '%s'", line);
            break;
        }
    }

    fclose(file);
}

void log_message(const char *fmt, ...) {
    FILE *file = fopen("a1.log", "a");

    if (!file) { die("log_message"); }

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

void clear_log(void) {
    FILE *file = fopen("a1.log", "w");
    fclose(file);
}
