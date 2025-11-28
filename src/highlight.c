#include "config.h"

#include "a1.h"
#include "syntaxes.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

bool is_separator(char ch) {
    return ch == SPACE || ch == '\0' || strchr(",.()+-/*=~%<>[];", ch) != NULL;
}

void editor_update_syntax_highlight(EditorRow *row) {
    // minimum of 1 required since setting a size of 0 frees the pointer
    row->highlight = realloc(row->highlight, MAX(row->render_size, 1));

    // fill row with 'normal' values
    memset(row->highlight, HL_NORMAL, row->render_size);

    if (editor_state.syntax == NULL) { return; }

    char **keywords = editor_state.syntax->keywords;
    char **types = editor_state.syntax->types;

    // single line comment start
    char *slcs = editor_state.syntax->single_line_comment_start;
    int slcs_len = slcs ? strlen(slcs) : 0;

    bool prev_sep = true; // beginning of line treated as separator

    bool in_str = false;
    char str_ch = '\0'; // wil be ', " or perhaps something else

    int i = 0;
    while (i < row->size) {
        char ch = row->render[i];
        unsigned char prev_hl = (i > 0) ? row->highlight[i - 1] : HL_NORMAL;

        // single line comment check
        if (slcs_len > 0 && !in_str) {
            if (strncmp(&row->render[i], slcs, slcs_len) == 0) {
                memset(&row->highlight[i], HL_COMMENT, row->render_size - i);
                break;
            }
        }

        // string check
        if (editor_state.syntax->flags & HL_HIGHLIGHT_STRINGS) {
            if (in_str) {
                row->highlight[i] = HL_STRING;

                // if string char escaped, skip
                if (ch == '\\' && i + 1 < row->render_size) {
                    row->highlight[i + 1] = HL_STRING;
                    i += 2;
                    continue;
                }

                if (ch == str_ch) { in_str = false; }
                i++;
                continue;

            } else {
                if (ch == '"' || ch == '\'') {
                    in_str = true;
                    str_ch = ch;
                    row->highlight[i] = HL_STRING;
                    i++;
                    continue;
                }
            }
        }

        // numeric literal check
        if (editor_state.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
            if ((isdigit(ch) && (prev_sep || prev_hl == HL_NUMBER)) ||
                (ch == '.' && prev_hl == HL_NUMBER)) {
                row->highlight[i] = HL_NUMBER;
                i++;
                prev_sep = false;
                continue;
            }
        }

        // keyword check
        if (prev_sep) {
            for (int j = 0; keywords[j]; j++) {
                int keyword_len = strlen(keywords[j]);
                // if space for keyword
                if (i + keyword_len < row->render_size) {
                    // string comparison
                    if (strncmp(&row->render[i], keywords[j], keyword_len) ==
                        0) {
                        // if keyword followed by separator
                        if (is_separator(row->render[i + keyword_len])) {
                            memset(&row->highlight[i], HL_KEYWORD, keyword_len);
                            i += keyword_len;
                            prev_sep = true;
                            continue;
                        }
                    }
                }
            }
        }

        // type check
        if (prev_sep) {
            for (int j = 0; types[j]; j++) {
                int keyword_len = strlen(types[j]);
                // if space for keyword
                if (i + keyword_len < row->render_size) {
                    // string comparison
                    if (strncmp(&row->render[i], types[j], keyword_len) ==
                        0) {
                        // if keyword followed by separator
                        if (is_separator(row->render[i + keyword_len])) {
                            memset(&row->highlight[i], HL_TYPE, keyword_len);
                            i += keyword_len;
                            prev_sep = true;
                            continue;
                        }
                    }
                }
            }
        }

        prev_sep = is_separator(ch);
        i++;
    }
}

void editor_update_syntax_highlight_all(void) {
    for (int row_index = 0; row_index < editor_state.num_rows; row_index++) {
        editor_update_syntax_highlight(&editor_state.rows[row_index]);
    }
}

// returns code(s) to be inserted into a select graphic rendition sequence
// e.g. \x1b[33m
char *editor_syntax_to_sequence(EditorHighlight highlight) {
    switch (highlight) {
    case HL_NUMBER:
        return "33"; // yellow
    case HL_STRING:
        return "32"; // green
    case HL_KEYWORD:
        return "35"; // magenta
    case HL_TYPE:
        return "36"; // cyan
    case HL_COMMENT:
        return "39;2"; // default fg color, dim
    case HL_MATCH:
        return "34"; // blue
    default:
        return "39"; // default fg color
    }
}

void editor_set_syntax_highlight(char *file_name) {
    editor_state.syntax = NULL;
    if (file_name == NULL) { return; }

    // get last occurence of char
    char *extension = strrchr(file_name, '.');

    for (unsigned int i = 0; i < ARRAY_LEN(syntax_db); i++) {
        EditorSyntax *syntax = &syntax_db[i];

        int j = 0;
        while (syntax->file_match[j]) {
            bool is_extension = syntax->file_match[j][0] == '.';
            bool match = false;

            // if is file extension based match in syntax db
            if (is_extension && extension &&
                strcmp(extension, syntax->file_match[j]) == 0) {
                match = true;
            }

            // if non file extension based match
            if (!is_extension &&
                strcmp(file_name, syntax->file_match[j]) == 0) {
                match = true;
            }

            if (match) {
                editor_state.syntax = syntax;
                editor_update_syntax_highlight_all();
                return;
            }

            j++;
        }
    }
}
