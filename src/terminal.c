#include "../include/config.h"

#include "../include/a1.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

void die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);   // clear entire screen
    write(STDOUT_FILENO, "\x1b[H", 3);    // move cursor to top-left
    write(STDOUT_FILENO, "\x1b[?25h", 6); // show cursor

    perror(s);
    exit(1);
}

void quit(void) {
    write(STDOUT_FILENO, "\x1b[2J", 4);   // clear screen
    write(STDOUT_FILENO, "\x1b[H", 3);    // move cursor to top-left
    write(STDOUT_FILENO, "\x1b[?25h", 6); // show cursor
    exit(0);
}

void disable_raw_mode(void) {
    int result =
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &editor_state.original_termios);
    if (result == -1) {
        die("tcsetattr");
    }
}

void enable_raw_mode(void) {
    int result = tcgetattr(STDIN_FILENO, &editor_state.original_termios);
    if (result == -1) {
        die("tcgetattr");
    }

    atexit(disable_raw_mode);

    // unreferenced bit flags are legacy
    // https://viewsourcecode.org/snaptoken/kilo/02.enteringRawMode.html#miscellaneous-flags

    struct termios new_termios = editor_state.original_termios;

    // disables the follow
    // ICRNL: Translate carriage return '\r' to '\n'
    // IXON: Transmission pausing/resume (C-S/C-Q)
    new_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    // disables the follow
    // OPOST: Output processing (e.g. translating '\n' to '\r\n')
    new_termios.c_oflag &= ~OPOST;
    new_termios.c_oflag |= CS8;

    // disables the following
    // ECHO: Echo output (write characters to input buffer)
    // ICANON: Canonical mode (read input line-by-line instead of byte-by-byte)
    // IEXTEN: C-V input
    // ISIG: C-C (interrupt) and C-Z (suspend) signals
    new_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    // VMIN: Minimum number of bytes required before read() can return
    // VTIME: The maximum amount of time to wait before read() returns
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 1; // 0.1s

    result = tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios);
    if (result == -1) {
        die("tcsetattr");
    }
}

int editor_read_key(void) {
    int bytes_read;
    char c;

    while ((bytes_read = read(STDIN_FILENO, &c, 1)) != 1) {
        // EAGAIN = "try again error" (required for Cygwin)
        if (bytes_read == -1 && errno != EAGAIN) {
            die("editor_read_key()");
        }
    }

    if (c == ESCAPE) {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) {
            return ESCAPE;
        }
        if (read(STDIN_FILENO, &seq[1], 1) != 1) {
            return ESCAPE;
        }

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) {
                    return ESCAPE;
                }
                if (seq[2] == '~') {
                    switch (seq[1]) {
                    case '1':
                        return HOME_KEY;
                    case '3':
                        return DEL_KEY;
                    case '4':
                        return END_KEY;
                    case '5':
                        return PAGE_UP;
                    case '6':
                        return PAGE_DOWN;
                    case '7':
                        return HOME_KEY;
                    case '8':
                        return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                case 'A':
                    return ARROW_UP;
                case 'B':
                    return ARROW_DOWN;
                case 'C':
                    return ARROW_RIGHT;
                case 'D':
                    return ARROW_LEFT;
                case 'H':
                    return HOME_KEY;
                case 'F':
                    return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
            case 'H':
                return HOME_KEY;
            case 'F':
                return END_KEY;
            }
        }
        return ESCAPE;
    } else {
        return c;
    }
}

int get_cursor_position(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    // report cursor position
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
        return -1;
    }

    // read response
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) {
            break;
        }
        if (buf[i] == 'R') {
            break;
        }
        i++;
    }
    buf[i] = '\0';

    // parse response
    if (buf[0] != ESCAPE || buf[1] != '[') {
        return -1;
    }
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) {
        return -1;
    }

    return 0;
}

int get_window_size(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        // if ioctl() fails, attempt querying terminal itself

        // go to right-most and bottom-most position
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
            return -1;
        }
        return get_cursor_position(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

// based on Neovim, decided by scroll offset not cursor position
void get_scroll_percentage(char *buf, size_t size) {
    if (editor_state.row_offset == 0) {
        strncpy(buf, "Top", size);
    }
    // -1 required for empty new line (~)
    else if (editor_state.num_rows - editor_state.screen_rows ==
             editor_state.row_offset) {
        strncpy(buf, "Bot", size);
    } else {
        double percentage =
            ((double)(editor_state.row_offset) /
             (editor_state.num_rows - editor_state.screen_rows)) *
            100;
        snprintf(buf, size, "%d%%", (int)percentage);
    }
}
