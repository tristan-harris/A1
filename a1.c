/*** includes ***/

// https://man7.org/linux/man-pages/man7/feature_test_macros.7.html
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
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

// strips every bit beyond fifth
// e.g. q (113) and Q (81) become C-Q/DC1 (17)
#define CTRL_KEY(k) ((k) & 0x1f)

typedef enum {
  ESCAPE = 27,
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
} EditorKey;

typedef enum {
  DIR_UP,
  DIR_RIGHT,
  DIR_LEFT,
  DIR_DOWN
} EditorDirection;

/*** data ***/

typedef struct {
  void (*entry_fn)(void *data);
  void (*input_fn)(int input);
  void (*exit_fn)();
  char *name;
} EditorMode;

typedef struct {
  char *text;
  int find_cursor_x;
  int find_cursor_y;
} EditorFindState;

typedef struct {
  int size; // size of row (excluding null character)
  int render_size; // size of rendered row
  char *chars; // row content
  char *render; // row content rendered to screen (needed for TABs)
} EditorRow;

typedef struct {
  int cursor_x; // actual cursor x position
  int cursor_y; // actual cursor y position
  int target_x; // intended x position of cursor based on previous line(s)
  int render_x; // rendered position of cursor
  int row_offset; // offset of rows displayed (vertical scroll)
  int col_offset; // offset of columns display (horizontal scroll)
  int screen_rows; // number of rows available in the emulator window
  int screen_cols; // number of columns available in the emulator window
  int num_rows; // number of rows that make up the text buffer
  EditorRow *rows;
  const EditorMode *mode; // normal, insert, command etc.
  EditorFindState find_state; // to store find mode variables
  bool dirty; // whether file has been modified since last write
  char *filename;
  char status_msg[80];
  time_t status_msg_time;
  struct termios original_termios;
} EditorState;

/*** prototypes ***/

void editor_set_status_message(const char *fmt, ...);
void editor_refresh_screen();
char *editor_prompt(char *prompt);
void editor_set_cursor_x(int x);
void editor_set_cursor_y(int x);

void normal_mode_entry(void *data);
void normal_mode_input(int input);
void normal_mode_exit();

void insert_mode_entry(void *data);
void insert_mode_input(int input);
void insert_mode_exit();

void command_mode_entry(void *data);
void command_mode_input(int input);
void command_mode_exit();

void find_mode_entry(void *data);
void find_mode_input(int input);
void find_mode_exit();

/*** global vars ***/

const EditorMode normal_mode = {
  .entry_fn = normal_mode_entry,
  .input_fn = normal_mode_input,
  .exit_fn = normal_mode_exit,
  .name = "NORMAL "
};

const EditorMode insert_mode = {
  .entry_fn = insert_mode_entry,
  .input_fn = insert_mode_input,
  .exit_fn = insert_mode_exit,
  .name = "INSERT "
};

const EditorMode command_mode = {
  .entry_fn = command_mode_entry,
  .input_fn = command_mode_input,
  .exit_fn = command_mode_exit,
  .name = "COMMAND"
};

const EditorMode find_mode = {
  .entry_fn = find_mode_entry,
  .input_fn = find_mode_input,
  .exit_fn = find_mode_exit,
  .name = "FIND   "
};


EditorState editor_state;

/*** terminal ***/

void die(const char *s) {
  write(STDOUT_FILENO, "\x1b[2J", 4); // clear entire screen
  write(STDOUT_FILENO, "\x1b[H", 3); // move cursor to top-left
  write(STDOUT_FILENO, "\x1b[?25h", 6); // show cursor

  perror(s);
  exit(1);
}

void quit() {
  write(STDOUT_FILENO, "\x1b[2J", 4); // clear screen
  write(STDOUT_FILENO, "\x1b[H", 3); // move cursor to top-left
  write(STDOUT_FILENO, "\x1b[?25h", 6); // show cursor
  exit(0);
}

void disable_raw_mode() {
  int result = tcsetattr(STDIN_FILENO, TCSAFLUSH, &editor_state.original_termios);
  if (result == -1) {
    die("tcsetattr");
  }
}

void enable_raw_mode() {
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

int editor_read_key() {
  int bytes_read;
  char c;

  while ((bytes_read = read(STDIN_FILENO, &c, 1)) != 1) {
    if (bytes_read == -1 && errno != EAGAIN) { // EAGAIN = "try again error" (required for Cygwin)
      die("editor_read_key()");
    }
  }

  if (c == ESCAPE) {
    char seq[3];

    if (read(STDIN_FILENO, &seq[0], 1) != 1) { return ESCAPE; }
    if (read(STDIN_FILENO, &seq[1], 1) != 1) { return ESCAPE; }

    if (seq[0] == '[') {
      if (seq[1] >= '0' && seq[1] <= '9') {
        if (read(STDIN_FILENO, &seq[2], 1) != 1) { return ESCAPE; }
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
    return ESCAPE;
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
  if (buf[0] != ESCAPE || buf[1] != '[') { return -1; }
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
  else if(editor_state.num_rows - editor_state.screen_rows == editor_state.row_offset) {
    strncpy(buf, "Bot", size);
  }
  else {
    double percentage = ((double)(editor_state.row_offset) / (editor_state.num_rows - editor_state.screen_rows)) * 100;
    snprintf(buf, size, "%d%%", (int)percentage);
  }
}

/*** row operations ***/

// convert cursor x position to equivalent rendered cursor x position
int editor_row_cx_to_rx(EditorRow *row, int cx) {
  int rx = 0;
  for (int i = 0; i < cx; i++) {
    if (row->chars[i] == '\t')
      rx += (A1_TAB_STOP - 1) - (rx % A1_TAB_STOP);
    rx++;
  }
  return rx;
}

// convert rendered cursor x position to equivalent cursor x position
int editor_row_rx_to_cx(EditorRow *row, int rx) {
  int current_rx = 0;
  int cx;

  for (cx = 0; cx < row->size; cx++) {
    if (row->chars[cx] == '\t') {
      current_rx += (A1_TAB_STOP - 1) - (current_rx % A1_TAB_STOP);
    }
    current_rx++;

    if (current_rx > rx) { return cx; }
  }

  return cx; // only needed when rx is out of range
}

void editor_update_row(EditorRow *row) {
  int tabs = 0;
  for (int i = 0; i < row->size; i++) {
    if (row->chars[i] == '\t') { tabs++; }
  }

  free(row->render);
  row->render = malloc(row->size + tabs * (A1_TAB_STOP - 1) + 1);

  int idx = 0;
  for (int i = 0; i < row->size; i++) {
    if (row->chars[i] == '\t') {
      row->render[idx++] = ' ';
      while (idx % A1_TAB_STOP != 0) { row->render[idx++] = ' '; }
    } else {
      row->render[idx++] = row->chars[i];
    }
  }
  row->render[idx] = '\0';
  row->render_size = idx;
}

void editor_insert_row(int row_idx, char *string, size_t len) {
  if (row_idx < 0 || row_idx > editor_state.num_rows) { return; }

  editor_state.rows = realloc(editor_state.rows, sizeof(EditorRow) * (editor_state.num_rows + 1));
  memmove(&editor_state.rows[row_idx + 1], &editor_state.rows[row_idx], sizeof(EditorRow) * (editor_state.num_rows - row_idx));

  editor_state.rows[row_idx].size = len;
  editor_state.rows[row_idx].chars = malloc(len + 1);
  memcpy(editor_state.rows[row_idx].chars, string, len);
  editor_state.rows[row_idx].chars[len] = '\0';

  editor_state.rows[row_idx].render_size = 0;
  editor_state.rows[row_idx].render = NULL;
  editor_update_row(&editor_state.rows[row_idx]);

  editor_state.num_rows++;
  editor_state.dirty = true;
}

void editor_free_row(EditorRow *row) {
  free(row->render);
  free(row->chars);
}

void editor_del_row(int row_idx) {
  if (row_idx < 0 || row_idx >= editor_state.num_rows) { return; }
  editor_free_row(&editor_state.rows[row_idx]);
  memmove(&editor_state.rows[row_idx], &editor_state.rows[row_idx + 1], sizeof(EditorRow) * (editor_state.num_rows - row_idx - 1));
  editor_state.num_rows--;
  editor_state.dirty = true;
}

void editor_row_insert_char(EditorRow *row, int col_idx, int c) {
  if (col_idx < 0 || col_idx > row->size) { col_idx = row->size; }
  row->chars = realloc(row->chars, row->size + 2);
  memmove(&row->chars[col_idx + 1], &row->chars[col_idx], row->size - col_idx + 1);
  row->size++;
  row->chars[col_idx] = c;
  editor_update_row(row);
  editor_state.dirty = true;
}

void editor_row_append_string(EditorRow *row, char *string, size_t len) {
  row->chars = realloc(row->chars, row->size + len + 1);
  memcpy(&row->chars[row->size], string, len);
  row->size += len;
  row->chars[row->size] = '\0';
  editor_update_row(row);
  editor_state.dirty = true;
}

void editor_row_del_char(EditorRow *row, int col_idx) {
  if (col_idx < 0 || col_idx >= row->size) { return; }
  memmove(&row->chars[col_idx], &row->chars[col_idx + 1], row->size - col_idx);
  row->size--;
  editor_update_row(row);
  editor_state.dirty = true;
}

void editor_row_invert_letter(EditorRow *row, int col_idx) {
  char c = row->chars[col_idx];

  // if uppercase or lower case letter
  if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122)) {
    row->chars[col_idx] ^= 0x20; // flip sixth bit to toggle case
  }
  editor_update_row(row);
}

/*** editor operations ***/

void editor_insert_char(int c) {
  if (editor_state.cursor_y == editor_state.num_rows) {
    editor_insert_row(editor_state.num_rows, "", 0);
  }
  editor_row_insert_char(&editor_state.rows[editor_state.cursor_y], editor_state.cursor_x, c);
  editor_state.cursor_x++;
}

void editor_insert_newline() {
  if (editor_state.cursor_x == 0) {
    editor_insert_row(editor_state.cursor_y, "", 0);
  } else {
    EditorRow *row = &editor_state.rows[editor_state.cursor_y];
    editor_insert_row(editor_state.cursor_y + 1, &row->chars[editor_state.cursor_x], row->size - editor_state.cursor_x);

    // pointer reassignment required because realloc() in editor_insert_row()
    // may move memory and invalidate the pointer
    row = &editor_state.rows[editor_state.cursor_y];

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

  EditorRow *row = &editor_state.rows[editor_state.cursor_y];
  if (editor_state.cursor_x > 0) {
    editor_row_del_char(row, editor_state.cursor_x - 1);
    editor_state.cursor_x--;
  } else {
    editor_state.cursor_x = editor_state.rows[editor_state.cursor_y - 1].size;
    editor_row_append_string(&editor_state.rows[editor_state.cursor_y - 1], row->chars, row->size);
    editor_del_row(editor_state.cursor_y);
    editor_state.cursor_y--;
  }
}

void editor_page_scroll(EditorDirection dir, bool half) {
  int scroll_amount = editor_state.screen_rows;
  if (half) { scroll_amount /= 2; }

  int new_y;

  switch (dir) {
    case DIR_UP:
      if (editor_state.cursor_y > 0) {
        new_y = editor_state.cursor_y - scroll_amount;
        if (new_y < 0) { new_y = 0; }
        editor_set_cursor_y(new_y);
      }
      break;

    case DIR_DOWN:
      if (editor_state.cursor_y < editor_state.num_rows - 1) {
        new_y = editor_state.cursor_y + scroll_amount;
        if (new_y >= editor_state.num_rows) { new_y = editor_state.num_rows - 1; }
        editor_set_cursor_y(new_y);
      }
      break;

    default:
      break;
  }
}

/*** file i/o ***/

char *editor_rows_to_string(int *buflen) {
  int total_len = 0;
  for (int i = 0; i < editor_state.num_rows; i++) {
    total_len += editor_state.rows[i].size + 1; // +1 for newline character
  }
  *buflen = total_len;

  char *buf = malloc(total_len);
  char *buf_p = buf;
  for (int i = 0; i < editor_state.num_rows; i++) {
    memcpy(buf_p, editor_state.rows[i].chars, editor_state.rows[i].size);
    buf_p += editor_state.rows[i].size;
    *buf_p = '\n';
    buf_p++;
  }

  return buf;
}

void editor_open(char *filename) {
  free(editor_state.filename);
  editor_state.filename = strdup(filename);

  FILE *fp = fopen(filename, "r");
  if (!fp) { die("fopen"); }

  char *line = NULL;
  size_t line_cap = 0;
  ssize_t line_len;
  while ((line_len = getline(&line, &line_cap, fp)) != -1) {
    // strip line-ending characters
    while (line_len > 0 && (line[line_len - 1] == '\n' || line[line_len - 1] == '\r')) {
      line_len--;
    }
    editor_insert_row(editor_state.num_rows, line, line_len);
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
    // ftruncate() adds 0 padding or deletes data to set file size same as len
    if (ftruncate(fd, len) != -1) {
      if (write(fd, buf, len) == len) {
        close(fd);
        free(buf);
        editor_state.dirty = false;
        editor_set_status_message("%d bytes written to disk.", len);
        return;
      }
    }
    close(fd);
  }

  free(buf);
  editor_set_status_message("Can't save! I/O error: %s", strerror(errno));
}

void editor_find() {
  char *query = editor_prompt("Find: %s");
  if (query == NULL) { return; }

  for (int i = 0; i < editor_state.num_rows; i++) {
    EditorRow *row = &editor_state.rows[i];
    char *match = strstr(row->render, query);
    if (match) {
      editor_set_cursor_y(i);
      editor_set_cursor_x(editor_row_rx_to_cx(row, match - row->render)); // pointer arithemtic

      // so that editor_render_scroll in the next update automatically
      // scrolls up to the cursor position on the next update
      editor_state.row_offset = editor_state.num_rows;

      break;
    }
  }

  free(query);
}

/*** append buffer ***/

// A very simple "append buffer" structure, that is a heap
// allocated string that can be appended to. This is useful in order to
// write all the escape sequences in a buffer and flush them to the standard
// output in a single call, to avoid flickering effects.

typedef struct {
  char *buf;
  int len;
} AppendBuffer;

void ab_append(AppendBuffer *ab, const char *string, int len) {
  char *new = realloc(ab->buf, ab->len + len);

  if (new == NULL) { return; }
  memcpy(&new[ab->len], string, len);
  ab->buf = new;
  ab->len += len;
}

void ab_free(AppendBuffer *ab) {
  free(ab->buf);
}

/*** output ***/

void editor_scroll_render_update() {
  editor_state.render_x = 0;

  if (editor_state.cursor_y < editor_state.num_rows) {
    editor_state.render_x = editor_row_cx_to_rx(&editor_state.rows[editor_state.cursor_y], editor_state.cursor_x);
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

void editor_draw_rows(AppendBuffer *ab) {
  for (int y = 0; y < editor_state.screen_rows; y++) {
    int filerow = y + editor_state.row_offset;

    // if past text buffer, fill lines below with '~'
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
      int len = editor_state.rows[filerow].render_size - editor_state.col_offset;
      if (len < 0) { len = 0; }
      if (len > editor_state.screen_cols) { len = editor_state.screen_cols; }

      // draw block cursor by inverting cell at cursor
      if (filerow == editor_state.cursor_y && editor_state.mode == &normal_mode) {
        // if blank line still draw block character
        if (len == 0) {
          ab_append(ab, "\x1b[7m \x1b[m", 8);
        } else {
          // FIX: col_offset crash
          ab_append(ab, &editor_state.rows[filerow].render[editor_state.col_offset], editor_state.render_x - editor_state.col_offset);
          ab_append(ab, "\x1b[7m", 4); // invert colors
          ab_append(ab, &editor_state.rows[filerow].render[editor_state.render_x], 1);
          ab_append(ab, "\x1b[m", 3); // reset formatting
          ab_append(ab, &editor_state.rows[filerow].render[editor_state.render_x + 1], len - editor_state.render_x - 1);
        }
      } else {
        ab_append(ab, &editor_state.rows[filerow].render[editor_state.col_offset], len);
      }
    }

    ab_append(ab, "\x1b[K", 3); // erase from active position to end of line
    ab_append(ab, "\r\n", 2);
  }
}

// TODO: improve after learning syntax highlighting
void editor_draw_status_bar(AppendBuffer *ab) {
  char left_status[80], right_status[80];

  // escape sequences bold mode text
  int left_len = snprintf(left_status, sizeof(left_status), "\x1b[1;7m%s\x1b[0;7m %.20s %s",
    editor_state.mode->name, editor_state.filename ? editor_state.filename : "[Unnamed]", editor_state.dirty ? "[Modified]" : "");
  int escape_sequence_char_count = 12;

  char scroll_percent[4]; // need to take null character (\0) into account
  get_scroll_percentage(scroll_percent, sizeof(scroll_percent));

  int right_len = snprintf(right_status, sizeof(right_status), "%d/%d, %d/%d (%s)",
    editor_state.cursor_y + 1, editor_state.num_rows, editor_state.cursor_x + 1, editor_state.rows[editor_state.cursor_y].size, scroll_percent);

  if (left_len > editor_state.screen_cols) { left_len = editor_state.screen_cols; }
  ab_append(ab, left_status, left_len);

  left_len -= escape_sequence_char_count;

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

void editor_draw_message_bar(AppendBuffer *ab) {
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
  editor_scroll_render_update();

  AppendBuffer ab = {.buf = NULL, .len = 0};

  if (editor_state.mode == &insert_mode) { ab_append(&ab, "\x1b[?25l", 6); } // hide cursor

  ab_append(&ab, "\x1b[H", 3); // move cursor to top-left

  editor_draw_rows(&ab);
  editor_draw_status_bar(&ab);
  editor_draw_message_bar(&ab);

  char buf[32];

  // move cursor to stored positon
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (editor_state.cursor_y - editor_state.row_offset) + 1,
            (editor_state.render_x - editor_state.col_offset) + 1);

  ab_append(&ab, buf, strlen(buf));

  if (editor_state.mode == &insert_mode) { ab_append(&ab, "\x1b[?25h", 6); } // show cursor

  write(STDOUT_FILENO, ab.buf, ab.len);
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
      if (buflen != 0) { buf[--buflen] = '\0'; }
    } else if (c == ESCAPE) {
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

void editor_set_cursor_x(int x) {
  editor_state.cursor_x = x;
  editor_state.target_x = x;
}

void editor_set_cursor_y(int y) {
  editor_state.cursor_y = y;
  EditorRow *row = &editor_state.rows[editor_state.cursor_y];
  if (editor_state.target_x > row->size - 1) {
    editor_state.cursor_x = row->size - 1;
  } else {
    editor_state.cursor_x = editor_state.target_x;
  }
}

void editor_move_cursor(EditorDirection dir) {
  EditorRow *row = &editor_state.rows[editor_state.cursor_y];

  switch (dir) {
    case DIR_UP:
      if (editor_state.cursor_y != 0) {
        editor_set_cursor_y(editor_state.cursor_y - 1);
      }
      break;

    case DIR_RIGHT:
      if (editor_state.mode == &normal_mode) {
        if (editor_state.cursor_x + 1 < row->size) {
          editor_set_cursor_x(editor_state.cursor_x + 1);
        }
      } else if (editor_state.mode == &insert_mode) {
        if (editor_state.cursor_x < row->size) {
          editor_set_cursor_x(editor_state.cursor_x + 1);
        }
      }
      break;

    case DIR_DOWN:
      if (editor_state.cursor_y + 1 < editor_state.num_rows) {
        editor_set_cursor_y(editor_state.cursor_y + 1);
      }
      break;

    case DIR_LEFT:
      if (editor_state.cursor_x != 0) {
        editor_set_cursor_x(editor_state.cursor_x - 1);
      }
      break;
  }
}

void editor_process_keypress() {
  int input = editor_read_key();
  editor_state.mode->input_fn(input);
}

/*** modes ***/

void transition_mode(const EditorMode *new_mode, void *data) {
  if (editor_state.mode != NULL) { editor_state.mode->exit_fn(); }
  editor_state.mode = new_mode;
  if (editor_state.mode != NULL) { editor_state.mode->entry_fn(data); }
}

void normal_mode_entry(void *data) {
  write(STDOUT_FILENO, "\x1b[?25l", 6); // hide cursor

  // move back from end of line (needed for transitioning from insert mode)
  if (editor_state.rows != NULL) {
    if (editor_state.cursor_x == editor_state.rows[editor_state.cursor_y].size) {
      editor_set_cursor_x(editor_state.cursor_x - 1);
    }
  }
}

void normal_mode_input(int input) {

  EditorRow *row = &editor_state.rows[editor_state.cursor_y];

  switch(input) {
    case CTRL_KEY('c'):
      editor_set_status_message("Press 'q' to quit after saving with 's', or 'Q' to quit without saving.");
      break;

    case CTRL_KEY('d'):
      editor_page_scroll(DIR_DOWN, true);
      break;
    case CTRL_KEY('u'):
      editor_page_scroll(DIR_UP, true);
      break;

    case CTRL_KEY('f'):
      editor_page_scroll(DIR_DOWN, false);
      break;
    case CTRL_KEY('b'):
      editor_page_scroll(DIR_UP, false);
      break;

    case ' ':
      transition_mode(&command_mode, NULL);
      break;

    case '0':
      editor_set_cursor_x(0);
      break;

    case '^':
      break;

    case '$':
      editor_set_cursor_x(row->size - 1);
      break;

    case 'a':
      transition_mode(&insert_mode, NULL);
      editor_move_cursor(DIR_RIGHT);
      break;
    case 'A':
      transition_mode(&insert_mode, NULL);
      editor_set_cursor_x(row->size);
      break;

    case 'd':
      editor_del_row(editor_state.cursor_y);
      break;

    case 'f':
      transition_mode(&command_mode, "Find: ");
      break;

    case 'g':
      editor_set_cursor_y(0);
      break;

    case 'G':
      editor_set_cursor_y(editor_state.num_rows - 1);
      break;

    case 'H':
      editor_set_cursor_y(editor_state.row_offset);
      break;
    case 'M':
      if (editor_state.row_offset + editor_state.screen_rows / 2 < editor_state.num_rows) {
        editor_set_cursor_y(editor_state.row_offset + editor_state.screen_rows / 2);
      } else {
        editor_set_cursor_y(editor_state.num_rows - 1);
      }
      break;
    case 'L':
      if (editor_state.row_offset + editor_state.screen_rows - 1 < editor_state.num_rows) {
        editor_set_cursor_y(editor_state.row_offset + editor_state.screen_rows - 1);
      } else {
        editor_set_cursor_y(editor_state.num_rows - 1);
      }
      break;

    case 'i':
      transition_mode(&insert_mode, NULL);
      break;
    case 'I':
      editor_set_cursor_x(0);
      transition_mode(&insert_mode, NULL);
      break;

    case 'h':
      editor_move_cursor(DIR_LEFT);
      break;
    case 'j':
      editor_move_cursor(DIR_DOWN);
      break;
    case 'k':
      editor_move_cursor(DIR_UP);
      break;
    case 'l':
      editor_move_cursor(DIR_RIGHT);
      break;

    case 'q':
      if (editor_state.dirty) {
        editor_set_status_message("File has unsaved changes.");
      } else { quit(); }
      break;
    case 'Q':
      quit();
      break;

    case 's':
      editor_save();
      break;

    case 'x':
      editor_row_del_char(row, editor_state.cursor_x);
      if (editor_state.cursor_x == row->size) {
        editor_set_cursor_x(editor_state.cursor_x - 1);
      }

    case '~':
      editor_row_invert_letter(row, editor_state.cursor_x);
      break;
  }
}

void normal_mode_exit() { }

void insert_mode_entry(void *data) {
  write(STDOUT_FILENO, "\x1b[?25h", 6); // show cursor
}

void insert_mode_input(int input) {
  switch (input) {
    case '\r':
      editor_insert_newline();
      break;

    case HOME_KEY:
      editor_set_cursor_x(0);
      break;

    case END_KEY:
      editor_set_cursor_x(editor_state.rows[editor_state.cursor_y].size);
      break;

    case BACKSPACE:
    case CTRL_KEY('h'):
    case DEL_KEY:
      if (input == DEL_KEY) { editor_move_cursor(DIR_RIGHT); }
      editor_del_char();
      break;

    case PAGE_UP:
    case PAGE_DOWN:
      {
        if (input == PAGE_UP) {
          editor_state.cursor_y = editor_state.row_offset;
        } else if (input == PAGE_DOWN) {
          editor_state.cursor_y = editor_state.row_offset + editor_state.screen_rows - 1;
          if (editor_state.cursor_y > editor_state.num_rows) editor_state.cursor_y = editor_state.num_rows;
        }

        int times = editor_state.screen_rows;
        while (times--)
          editor_move_cursor(input == PAGE_UP ? DIR_UP : DIR_DOWN);
      }
      break;

    case ARROW_UP:
      editor_move_cursor(DIR_UP);
      break;
    case ARROW_DOWN:
      editor_move_cursor(DIR_DOWN);
      break;
    case ARROW_LEFT:
      editor_move_cursor(DIR_LEFT);
      break;
    case ARROW_RIGHT:
      editor_move_cursor(DIR_RIGHT);
      break;

    case CTRL_KEY('c'):
    case CTRL_KEY('l'):
    case ESCAPE:
      transition_mode(&normal_mode, NULL);
      break;

    default:
      editor_insert_char(input);
      break;
  }
}

void insert_mode_exit() { }

void command_mode_entry(void *data) {

}

void command_mode_input(int input) {
  switch (input) {
    case ESCAPE:
      transition_mode(&normal_mode, NULL);
      break;
    case '\r':
      transition_mode(&normal_mode, NULL);
      break;
  }
}

void command_mode_exit() {

}

void find_mode_entry(void *data) { }

void find_mode_input(int input) {
  switch (input) {

  }
}

void find_mode_exit() { }

/*** init ***/

void init_editor() {
  editor_state.cursor_x = 0;
  editor_state.cursor_y = 0;
  editor_state.target_x = 0;
  editor_state.render_x = 0;
  editor_state.row_offset = 0;
  editor_state.col_offset = 0;
  editor_state.num_rows = 0;
  editor_state.rows = NULL;

  editor_state.find_state.text = NULL;
  editor_state.find_state.find_cursor_x = 0;
  editor_state.find_state.find_cursor_y = 0;

  editor_state.dirty = false;
  editor_state.mode = NULL;
  editor_state.filename = NULL;
  editor_state.status_msg[0] = '\0';
  editor_state.status_msg_time = 0;

  int result = get_window_size(&editor_state.screen_rows, &editor_state.screen_cols);
  if (result == -1) {
    die("getWindowSize");
  }

  editor_state.screen_rows -= 2; // accounting for status bar and message rows

  transition_mode(&normal_mode, NULL); // start editor in normal mode
}

int main(int argc, char *argv[]) {
  enable_raw_mode();
  init_editor();

  if (argc >= 2) {
    editor_open(argv[1]);
  } else {
    editor_insert_row(0, "", 0); // empty new file
  }

  while (true) {
    editor_refresh_screen();
    editor_process_keypress();
  }

  return EXIT_SUCCESS;
}
