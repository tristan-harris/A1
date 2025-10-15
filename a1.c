/*** includes ***/

// https://man7.org/linux/man-pages/man7/feature_test_macros.7.html
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/*** defines ***/

#define A1_VERSION "0.0.1"
#define A1_TAB_STOP 4
#define A1_QUIT_TIMES 1

// strips every bit beyond fifth
// e.g. q (113) and Q (81) become C-Q/DC1 (17)
#define CTRL_KEY(k) ((k) & 0x1f)

enum editor_key_t {
  BACKSPACE = 127,

  // soft codes, not related to ASCII values
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
  DEL_KEY,
  HOME_KEY,
  END_KEY,
  PAGE_UP,
  PAGE_DOWN
};

/*** data ***/

typedef struct editor_row {
  int size; // size of row (excluding null character)
  int r_size; // size of rendered row
  char *chars; // row content
  char *render; // row content rendered to screen (needed for TABs)
} editor_row_t;

typedef struct editor_state {
  int cursor_x, cursor_y; // cursor position
  int render_x; // rendered position of cursor
  int row_offset; // offset of rows displayed (vertical scroll)
  int col_offset; // offset of columns display (horizontal scroll)
  int screen_rows; // number of rows available in the emulator window
  int screen_cols; // number of columns available in the emulator window
  int num_rows; // number of rows that make up the text buffer
  editor_row_t *row; // rows
  bool dirty; // whether file has been modified since last write
  char *filename;
  char status_msg[80];
  time_t status_msg_time;
  struct termios orig_termios;
} editor_state_t;

editor_state_t editor_state;

/*** prototypes ***/

void editor_set_status_message(const char *fmt, ...);
void editor_refresh_screen();
char *editor_prompt(char *prompt);

/*** terminal ***/

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4); // clear entire screen
  write(STDOUT_FILENO, "\x1b[H", 3); // move cursor (default of top-left)

  perror(s);
  exit(1);
}

void disable_raw_mode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &editor_state.orig_termios) == -1) {
    die("tcsetattr");
  }
}

void enable_raw_mode() {
  if (tcgetattr(STDIN_FILENO, &editor_state.orig_termios) == -1) {
    die("tcgetattr");
  }
  atexit(disable_raw_mode);

  struct termios raw = editor_state.orig_termios;
  raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
    die("tcsetattr");
  }
}

int editor_read_key() {
  int bytes_read;
  char c;
  while ((bytes_read = read(STDIN_FILENO, &c, 1)) != 1) {
    if (bytes_read == -1 && errno != EAGAIN) {
      die("read");
    }
  }

  if (c == '\x1b') {
    char seq[3];

    if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

    if (seq[0] == '[') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
        if (seq[2] == '~') {
          switch (seq[1]) {
            case '1': return HOME_KEY;
            case '3': return DEL_KEY;
            case '4': return END_KEY;
            case '5': return PAGE_UP;
            case '6': return PAGE_DOWN;
            case '7': return HOME_KEY;
            case '8': return END_KEY;
          }
        }
      } else {
        switch (seq[1]) {
          case 'A': return ARROW_UP;
          case 'B': return ARROW_DOWN;
          case 'C': return ARROW_RIGHT;
          case 'D': return ARROW_LEFT;
          case 'H': return HOME_KEY;
          case 'F': return END_KEY;
        }
      }
    } else if (seq[0] == 'O') {
      switch (seq[1]) {
        case 'H': return HOME_KEY;
        case 'F': return END_KEY;
      }
    }

    return '\x1b';
  } else {
    return c;
  }
}

int get_cursor_position(int *rows, int *cols) {
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
  if (buf[0] != '\x1b' || buf[1] != '[') { return -1; }
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) { return -1; }

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
  else if(editor_state.num_rows - editor_state.screen_rows == editor_state.row_offset - 1) {
    strncpy(buf, "Bot", size);
  }
  else {
    double percentage = ((double)(editor_state.row_offset - 1) / (editor_state.num_rows - editor_state.screen_rows)) * 100;
    snprintf(buf, size, "%d%%", (int)percentage);
  }
}

/*** row operations ***/

int editor_row_cx_to_rx(editor_row_t *row, int cx) {
  int rx = 0;
  for (int j = 0; j < cx; j++) {
    if (row->chars[j] == '\t')
      rx += (A1_TAB_STOP - 1) - (rx % A1_TAB_STOP);
    rx++;
  }
  return rx;
}

void editor_update_row(editor_row_t *row) {
  int tabs = 0;
  for (int j = 0; j < row->size; j++) {
    if (row->chars[j] == '\t') { tabs++; }
  }

  free(row->render);
  row->render = malloc(row->size + tabs*(A1_TAB_STOP - 1) + 1);

  int idx = 0;
  for (int j = 0; j < row->size; j++) {
    if (row->chars[j] == '\t') {
      row->render[idx++] = ' ';
      while (idx % A1_TAB_STOP != 0) { row->render[idx++] = ' '; }
    } else {
      row->render[idx++] = row->chars[j];
    }
  }
  row->render[idx] = '\0';
  row->r_size = idx;
}

void editor_insert_row(int at, char *s, size_t len) {
  if (at < 0 || at > editor_state.num_rows) { return; }

  editor_state.row = realloc(editor_state.row, sizeof(editor_row_t) * (editor_state.num_rows + 1));
  memmove(&editor_state.row[at + 1], &editor_state.row[at], sizeof(editor_row_t) * (editor_state.num_rows - at));

  editor_state.row[at].size = len;
  editor_state.row[at].chars = malloc(len + 1);
  memcpy(editor_state.row[at].chars, s, len);
  editor_state.row[at].chars[len] = '\0';

  editor_state.row[at].r_size = 0;
  editor_state.row[at].render = NULL;
  editor_update_row(&editor_state.row[at]);

  editor_state.num_rows++;
  editor_state.dirty = true;
}

void editor_free_row(editor_row_t *row) {
  free(row->render);
  free(row->chars);
}

void editor_del_row(int at) {
  if (at < 0 || at >= editor_state.num_rows) { return; }
  editor_free_row(&editor_state.row[at]);
  memmove(&editor_state.row[at], &editor_state.row[at + 1], sizeof(editor_row_t) * (editor_state.num_rows - at - 1));
  editor_state.num_rows--;
  editor_state.dirty = true;
}

void editor_row_insert_char(editor_row_t *row, int at, int c) {
  if (at < 0 || at > row->size) { at = row->size; }
  row->chars = realloc(row->chars, row->size + 2);
  memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
  row->size++;
  row->chars[at] = c;
  editor_update_row(row);
  editor_state.dirty = true;
}

void editor_row_append_string(editor_row_t *row, char *s, size_t len) {
  row->chars = realloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], s, len);
  row->size += len;
  row->chars[row->size] = '\0';
  editor_update_row(row);
  editor_state.dirty = true;
}

void editor_row_del_char(editor_row_t *row, int at) {
  if (at < 0 || at >= row->size) { return; }
  memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
  row->size--;
  editor_update_row(row);
  editor_state.dirty = true;
}

/*** editor operations ***/

void editor_insert_char(int c) {
  if (editor_state.cursor_y == editor_state.num_rows) {
    editor_insert_row(editor_state.num_rows, "", 0);
  }
  editor_row_insert_char(&editor_state.row[editor_state.cursor_y], editor_state.cursor_x, c);
  editor_state.cursor_x++;
}

void editor_insert_newline() {
  if (editor_state.cursor_x == 0) {
    editor_insert_row(editor_state.cursor_y, "", 0);
  } else {
    editor_row_t *row = &editor_state.row[editor_state.cursor_y];
    editor_insert_row(editor_state.cursor_y + 1, &row->chars[editor_state.cursor_x], row->size - editor_state.cursor_x);
    row = &editor_state.row[editor_state.cursor_y];
    row->size = editor_state.cursor_x;
    row->chars[row->size] = '\0';
    editor_update_row(row);
  }
  editor_state.cursor_y++;
  editor_state.cursor_x = 0;
}

void editor_del_char() {
  if (editor_state.cursor_y == editor_state.num_rows) { return; }
  if (editor_state.cursor_x == 0 && editor_state.cursor_y == 0) { return; }

  editor_row_t *row = &editor_state.row[editor_state.cursor_y];
  if (editor_state.cursor_x > 0) {
    editor_row_del_char(row, editor_state.cursor_x - 1);
    editor_state.cursor_x--;
  } else {
    editor_state.cursor_x = editor_state.row[editor_state.cursor_y - 1].size;
    editor_row_append_string(&editor_state.row[editor_state.cursor_y - 1], row->chars, row->size);
    editor_del_row(editor_state.cursor_y);
    editor_state.cursor_y--;
  }
}

/*** file i/o ***/

char *editor_rows_to_string(int *buflen) {
  int totlen = 0;
  for (int j = 0; j < editor_state.num_rows; j++) {
    totlen += editor_state.row[j].size + 1;
  }
  *buflen = totlen;

  char *buf = malloc(totlen);
  char *p = buf;
  for (int j = 0; j < editor_state.num_rows; j++) {
    memcpy(p, editor_state.row[j].chars, editor_state.row[j].size);
    p += editor_state.row[j].size;
    *p = '\n';
    p++;
  }

  return buf;
}

void editor_open(char *filename) {
  free(editor_state.filename);
  editor_state.filename = strdup(filename);

  FILE *fp = fopen(filename, "r");
  if (!fp) { die("fopen"); }

  char *line = NULL;
  size_t linecap = 0;
  ssize_t linelen;
  while ((linelen = getline(&line, &linecap, fp)) != -1) {
    while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) {
      linelen--;
    }
    editor_insert_row(editor_state.num_rows, line, linelen);
  }
  free(line);
  fclose(fp);
  editor_state.dirty = false;
}

void editor_save() {
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
    if (ftruncate(fd, len) != -1) {
      if (write(fd, buf, len) == len) {
        close(fd);
        free(buf);
        editor_state.dirty = false;
        editor_set_status_message("%d bytes written to disk", len);
        return;
      }
    }
    close(fd);
  }

  free(buf);
  editor_set_status_message("Can't save! I/O error: %s", strerror(errno));
}

/*** append buffer ***/

struct append_buffer {
  char *b;
  int len;
};

#define ABUF_INIT {NULL, 0}

void ab_append(struct append_buffer *ab, const char *s, int len) {
  char *new = realloc(ab->b, ab->len + len);

  if (new == NULL) { return; }
  memcpy(&new[ab->len], s, len);
  ab->b = new;
  ab->len += len;
}

void ab_free(struct append_buffer *ab) {
  free(ab->b);
}

/*** output ***/

void editor_scroll() {
  editor_state.render_x = 0;
  if (editor_state.cursor_y < editor_state.num_rows) {
    editor_state.render_x = editor_row_cx_to_rx(&editor_state.row[editor_state.cursor_y], editor_state.cursor_x);
  }

  if (editor_state.cursor_y < editor_state.row_offset) {
    editor_state.row_offset = editor_state.cursor_y;
  }
  if (editor_state.cursor_y >= editor_state.row_offset + editor_state.screen_rows) {
    editor_state.row_offset = editor_state.cursor_y - editor_state.screen_rows + 1;
  }
  if (editor_state.render_x < editor_state.col_offset) {
    editor_state.col_offset = editor_state.render_x;
  }
  if (editor_state.render_x >= editor_state.col_offset + editor_state.screen_cols) {
    editor_state.col_offset = editor_state.render_x - editor_state.screen_cols + 1;
  }
}

void editor_draw_rows(struct append_buffer *ab) {
  for (int y = 0; y < editor_state.screen_rows; y++) {
    int filerow = y + editor_state.row_offset;
    if (filerow >= editor_state.num_rows) {
      if (editor_state.num_rows == 0 && y == editor_state.screen_rows / 3) {
        char welcome[80];
        int welcomelen = snprintf(welcome, sizeof(welcome), "A1 editor -- version %s", A1_VERSION);
        if (welcomelen > editor_state.screen_cols) welcomelen = editor_state.screen_cols;
        int padding = (editor_state.screen_cols - welcomelen) / 2;
        if (padding) {
          ab_append(ab, "~", 1);
          padding--;
        }
        while (padding--) { ab_append(ab, " ", 1); }
        ab_append(ab, welcome, welcomelen);
      } else {
        ab_append(ab, "~", 1);
      }
    } else {
      int len = editor_state.row[filerow].r_size - editor_state.col_offset;
      if (len < 0) { len = 0; }
      if (len > editor_state.screen_cols) { len = editor_state.screen_cols; }
      ab_append(ab, &editor_state.row[filerow].render[editor_state.col_offset], len);
    }

    ab_append(ab, "\x1b[K", 3); // erase from active position to end of line
    ab_append(ab, "\r\n", 2);
  }
}

void editor_draw_status_bar(struct append_buffer *ab) {
  ab_append(ab, "\x1b[7m", 4); // invert colors
  char left_status[80], right_status[80];

  int left_len = snprintf(left_status, sizeof(left_status), "%.20s %s",
    editor_state.filename ? editor_state.filename : "[No Name]", editor_state.dirty ? "[+]" : "");

  char scroll_percent[4]; // need to take null character (\0) into account
  get_scroll_percentage(scroll_percent, sizeof(scroll_percent));

  int right_len = snprintf(right_status, sizeof(right_status), "%d,%d %s",
    editor_state.cursor_y + 1, editor_state.cursor_x + 1, scroll_percent);

  if (left_len > editor_state.screen_cols) { left_len = editor_state.screen_cols; }
  ab_append(ab, left_status, left_len);

  while (left_len < editor_state.screen_cols) {
    if (editor_state.screen_cols - left_len == right_len) {
      ab_append(ab, right_status, right_len);
      break;
    } else {
      ab_append(ab, " ", 1);
      left_len++;
    }
  }

  ab_append(ab, "\x1b[m", 3); // reset formatting
  ab_append(ab, "\r\n", 2);
}

void editor_draw_message_bar(struct append_buffer *ab) {
  ab_append(ab, "\x1b[K", 3); // erase from active position to end of line
  int msglen = strlen(editor_state.status_msg);
  if (msglen > editor_state.screen_cols) {
    msglen = editor_state.screen_cols;
  }
  if (msglen && time(NULL) - editor_state.status_msg_time < 5) {
    ab_append(ab, editor_state.status_msg, msglen);
  }
}

void editor_refresh_screen() {
  editor_scroll();

  struct append_buffer ab = ABUF_INIT;

  ab_append(&ab, "\x1b[?25l", 6); // hide cursor
  ab_append(&ab, "\x1b[H", 3); // move cursor to top-left

  editor_draw_rows(&ab);
  editor_draw_status_bar(&ab);
  editor_draw_message_bar(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (editor_state.cursor_y - editor_state.row_offset) + 1,
                                            (editor_state.render_x - editor_state.col_offset) + 1);
  ab_append(&ab, buf, strlen(buf));

  ab_append(&ab, "\x1b[?25h", 6); // show cursor

  write(STDOUT_FILENO, ab.b, ab.len);
  ab_free(&ab);
}

void editor_set_status_message(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(editor_state.status_msg, sizeof(editor_state.status_msg), fmt, ap);
  va_end(ap);
  editor_state.status_msg_time = time(NULL);
}

/*** input ***/

char *editor_prompt(char *prompt) {
  size_t bufsize = 128;
  char *buf = malloc(bufsize);

  size_t buflen = 0;
  buf[0] = '\0';

  while (true) {
    editor_set_status_message(prompt, buf);
    editor_refresh_screen();

    int c = editor_read_key();
    if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
      if (buflen != 0) buf[--buflen] = '\0';
    } else if (c == '\x1b') {
      editor_set_status_message("");
      free(buf);
      return NULL;
    } else if (c == '\r') {
      if (buflen != 0) {
        editor_set_status_message("");
        return buf;
      }
    } else if (!iscntrl(c) && c < 128) {
      if (buflen == bufsize - 1) {
        bufsize *= 2;
        buf = realloc(buf, bufsize);
      }
      buf[buflen++] = c;
      buf[buflen] = '\0';
    }
  }
}

void editor_move_cursor(int key) {
  editor_row_t *row = (editor_state.cursor_y >= editor_state.num_rows) ? NULL : &editor_state.row[editor_state.cursor_y];

  switch (key) {
    case ARROW_LEFT:
      if (editor_state.cursor_x != 0) {
        editor_state.cursor_x--;
      } else if (editor_state.cursor_y > 0) {
        editor_state.cursor_y--;
        editor_state.cursor_x = editor_state.row[editor_state.cursor_y].size;
      }
      break;
    case ARROW_RIGHT:
      if (row && editor_state.cursor_x < row->size) {
        editor_state.cursor_x++;
      } else if (row && editor_state.cursor_x == row->size) {
        editor_state.cursor_y++;
        editor_state.cursor_x = 0;
      }
      break;
    case ARROW_UP:
      if (editor_state.cursor_y != 0) {
        editor_state.cursor_y--;
      }
      break;
    case ARROW_DOWN:
      if (editor_state.cursor_y < editor_state.num_rows) {
        editor_state.cursor_y++;
      }
      break;
  }

  row = (editor_state.cursor_y >= editor_state.num_rows) ? NULL : &editor_state.row[editor_state.cursor_y];
  int rowlen = row ? row->size : 0;
  if (editor_state.cursor_x > rowlen) {
    editor_state.cursor_x = rowlen;
  }
}

void editor_process_keypress() {
  static int quit_times = A1_QUIT_TIMES;

  int c = editor_read_key();

  switch (c) {
    case '\r':
      editor_insert_newline();
      break;

    case CTRL_KEY('q'):
      if (editor_state.dirty && quit_times > 0) {
        editor_set_status_message("WARNING!!! File has unsaved changes. "
          "Press Ctrl-Q %d more times to quit.", quit_times);
        quit_times--;
        return;
      }
      write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
      write(STDOUT_FILENO, "\x1b[H", 3); // move cursor to top-left
      exit(0);
      break;

    case CTRL_KEY('s'):
      editor_save();
      break;

    case HOME_KEY:
      editor_state.cursor_x = 0;
      break;

    case END_KEY:
      if (editor_state.cursor_y < editor_state.num_rows)
        editor_state.cursor_x = editor_state.row[editor_state.cursor_y].size;
      break;

    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
      if (c == DEL_KEY) editor_move_cursor(ARROW_RIGHT);
      editor_del_char();
      break;

    case PAGE_UP:
    case PAGE_DOWN:
      {
        if (c == PAGE_UP) {
          editor_state.cursor_y = editor_state.row_offset;
        } else if (c == PAGE_DOWN) {
          editor_state.cursor_y = editor_state.row_offset + editor_state.screen_rows - 1;
          if (editor_state.cursor_y > editor_state.num_rows) editor_state.cursor_y = editor_state.num_rows;
        }

        int times = editor_state.screen_rows;
        while (times--)
          editor_move_cursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
      }
      break;

    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      editor_move_cursor(c);
      break;

    case CTRL_KEY('l'):
    case '\x1b':
      break;

    default:
      editor_insert_char(c);
      break;
  }

  quit_times = A1_QUIT_TIMES;
}

/*** init ***/

void init_editor() {
  editor_state.cursor_x = 0;
  editor_state.cursor_y = 0;
  editor_state.render_x = 0;
  editor_state.row_offset = 0;
  editor_state.col_offset = 0;
  editor_state.num_rows = 0;
  editor_state.row = NULL;
  editor_state.dirty = false;
  editor_state.filename = NULL;
  editor_state.status_msg[0] = '\0';
  editor_state.status_msg_time = 0;

  if (get_window_size(&editor_state.screen_rows, &editor_state.screen_cols) == -1) {
    die("getWindowSize");
  }
  editor_state.screen_rows -= 2;
}

int main(int argc, char *argv[]) {
  enable_raw_mode();
  init_editor();
  if (argc >= 2) {
    editor_open(argv[1]);
  }

  editor_set_status_message("HELP: Ctrl-S = save | Ctrl-Q = quit");

  while (true) {
    editor_refresh_screen();
    editor_process_keypress();
  }

  return EXIT_SUCCESS;
}
