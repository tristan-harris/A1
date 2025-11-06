#include "../include/config.h"

#include "../include/a1.h"
#include "../include/editor.h"
#include "../include/terminal.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

EditorState editor_state;

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
        editor_set_status_message("%d", editor_state.cursor_x);
    }

    return EXIT_SUCCESS;
}
