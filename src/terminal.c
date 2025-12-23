#include "a1.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

void terminal_die(const char *s) {
    write(STDOUT_FILENO, "\x1b[2J", 4);   // clear entire screen
    write(STDOUT_FILENO, "\x1b[H", 3);    // move cursor to top-left
    write(STDOUT_FILENO, "\x1b[?25h", 6); // show cursor

    perror(s);
    exit(EXIT_FAILURE);
}

void terminal_quit(void) {
    write(STDOUT_FILENO, "\x1b[2J", 4);   // clear screen
    write(STDOUT_FILENO, "\x1b[H", 3);    // move cursor to top-left
    write(STDOUT_FILENO, "\x1b[?25h", 6); // show cursor
    exit(EXIT_SUCCESS);
}

void terminal_disable_raw_mode(void) {
    int result =
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &editor_state.original_termios);
    if (result == -1) { terminal_die("tcsetattr"); }
}

void terminal_enable_raw_mode(void) {
    int result = tcgetattr(STDIN_FILENO, &editor_state.original_termios);
    if (result == -1) { terminal_die("tcgetattr"); }

    atexit(terminal_disable_raw_mode);

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
    if (result == -1) { terminal_die("tcsetattr"); }
}

int terminal_get_cursor_position(int *rows, int *cols) {
    char buf[32];
    unsigned int i = 0;

    // report cursor position
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) { return -1; }

    // read response
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) { break; }
        if (buf[i] == 'R') { break; }
        i++;
    }
    buf[i] = '\0';

    // parse response
    if (buf[0] != ESCAPE || buf[1] != '[') { return -1; }
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) { return -1; }

    return 0;
}

int terminal_get_window_size(int *rows, int *cols) {
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        // if ioctl() fails, attempt querying terminal itself

        // go to right-most and bottom-most position
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) { return -1; }
        return terminal_get_cursor_position(rows, cols);
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

void terminal_update_window_size(void) {
    int result = terminal_get_window_size(&editor_state.screen_rows,
                                          &editor_state.screen_cols);
    if (result == -1) { terminal_die("getWindowSize"); }

    // needed to account for status bar and message rows
    editor_state.screen_rows -= 2;
}
