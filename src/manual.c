#include "config.h"

#include "a1.h"
#include "manual.h"
#include <stddef.h>

const char *manual_text[] = {
    "=== A1 (v" A1_VERSION /* NOLINT(bugprone-suspicious-missing-comma) */
    ") ===",
    "A vim-based terminal text editor.\n",

    "=== OVERVIEW ===",
    "Like vim, A1 is a modal text editor. This means you switch between",
    "different modes depending on what you want to do. The current mode is",
    "shown at the bottom left of the screen on the status line.\n",

    "To use A1 as a basic text editor, press 'i' to enter INSERT mode and",
    "type text as you normally would. From INSERT MODE you can press 'Escape'",
    "to exit to NORMAL mode. From NORMAL mode you can save with 's', quit",
    "with 'q' or forcefully quit with 'Q'.\n",

    "=== MODES ===",
    "A1 has four modes, with their purposes described below\n",

    "NORMAL:  To navigate the text buffer",
    "INSERT:  To modify the text buffer",
    "COMMAND: To enter various editor commands",
    "FIND:    To find matches in the text buffer\n",

    "These modes are described in greater detail below.\n",

    "=== NORMAL MODE ===",
    "Normal mode has the following key-bindings",
    "(C-d for instance means Control + d).\n",

    "h      -> Move left",
    "j      -> Move down",
    "k      -> Move up",
    "l      -> Move right\n",

    "s      -> Save (write text buffer to file)",
    "q      -> Quit (will cancel if buffer is modified)",
    "Q      -> Force quit\n",

    "C-d    -> Move down half a page",
    "C-u    -> Move up half a page",
    "C-f    -> Move down a full page",
    "C-b    -> Move up a full page",
    "C-h    -> Jump to beginning of visible line",
    "C-l    -> Jump to end of visible line\n",

    "H      -> Jump to first visible line",
    "M      -> Jump to halfway between first and last visible lines",
    "L      -> Jump to last visible line\n",

    "Space  -> Enter COMMAND mode",
    "f      -> Enter COMMAND mode with 'find' prompt",
    "n      -> Enter COMMAND mode with 'goto' prompt",
    "S      -> Enter COMMAND mode with 'set' prompt\n",

    "a      -> Enter INSERT mode to the right",
    "A      -> Enter INSERT mode at end of line",
    "i      -> Enter INSERT mode to the left",
    "I      -> Enter INSERT mode at beginning of line\n",

    "0      -> Jump to beginning of line",
    "^      -> Jump to first non-whitespace character of line",
    "$      -> Jump to end of line",
    "{      -> Jump to previous blank line",
    "}      -> Jump to next blank line\n",

    "b      -> Jump to beginning of previous word",
    "e      -> Jump to end of next word",
    "w      -> Jump to beginning of next word",
    "g      -> Jump to first line",
    "G      -> Jump to last line\n",

    "d      -> Delete line",
    "D      -> Delete to end of line",
    "x      -> Delete character under cursor\n",

    "o      -> Insert new line below and enter INSERT mode there",
    "O      -> Insert new line above and enter INSERT mode there\n",

    "c      -> Centre view vertically",
    "~      -> Invert case of character under cursor\n",

    "=== INSERT MODE ===",
    "In INSERT mode you can modify text as you would in any other text editor.",
    "The following key-bindings are in addition to that.\n",

    "ESCAPE -> Exit INSERT MODE to NORMAL mode",
    "C-c    -> Same as ESCAPE\n",

    "HOME   -> Move to beginning of line",
    "END    -> Move to end of line\n",

    "PAGEUP -> Move up half a page",
    "PAGEDN -> Move down half a page\n",

    "=== COMMAND MODE ===",
    "From here, you can enter various editor commands. These commands and",
    "their arguments listed below.\n",

    "Arguments surrounded by square brackets are optional.\n",

    "save [FILE] -> Save text buffer\n",

    "find STRING [i/I] -> Enter FIND mode and search for STRING.",
    "- Case insensitive/sensitive search can be set by adding i/I afterwards",
    "- STRING can contain whitespace by using '\\ ' for SPACE",
    "  and '\\t' for TAB\n",
    "- Example: 'find text\\ editor' to find occurrences of 'text editor'\n",

    "goto LINE_NUMBER -> Jump to specified line number.\n",

    "get OPTION -> Get the current value of an editor option.",
    "set OPTION VALUE -> Set the value of an editor option.\n",

    "=== FIND MODE ===",
    "Here you can jump between matches of the searched string.\n",

    "p      -> Jump to previous match",
    "n      -> Jump to next match\n",

    "ENTER  -> Exit to NORMAL mode with cursor moved to current match",
    "ESCAPE -> Exit to NORMAL mode with cursor returned to original position",
    "q      -> Same as ESCAPE\n",

    "c      -> Centre view vertically (like NORMAL mode)\n",

    "=== CONFIGURATION ===",
    "A1 can be configured by setting various options.\n",

    "'autoindent' (alias 'ai') -> Whether to automatically indent a newly ",
    "inserted line based on how much the line above is indented. Default ",
    "'true'.\n",

    "'caseinsensitivedefault' (alias 'cid') -> Whether to perform a",
    "case-insensitive search by default in FIND mode. Default 'true'.",
    "Can be overriden by specifying i/I in command.\n",

    "'linenumber' (alias 'ln') -> Whether to show line numbers.",
    "Default 'true'.\n",

    "'tabcharacter' (alias 'tc') -> Whether to insert a literal tab-character",
    "when the TAB key is pressed (spaces otherwise). Default 'false'.\n",

    "'tabstop' (alias 'ts') -> How far to indent a line when TAB is pressed",
    "Default '4'.\n",

    "In COMMAND mode you can set these options using the 'set' command.",
    "You can also set them more permanently by writing 'set' commands in",
    "a configuration file, which is executed automatically upon starting.\n",

    "A1 first looks for a configuration file at $XDG_CONFIG_HOME/a1/a1rc if",
    "$XDG_CONFIG_HOME is set. Otherwise it tries $HOME/.config/a1/a1rc.\n",

    "If a config file is found there, A1 will execute every 'set' command",
    "in the file. Anything else other than comments (prefixed with '#') will",
    "prevent any further execution of the commands in the file.\n",

    "=== OTHER ===",
    "A1 has basic syntax highlighting support for C and Python.\n"};

const int manual_text_len = ARRAY_LEN(manual_text);
