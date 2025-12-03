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

    char **highlight_words[] = {editor_state.syntax->keywords,
                                editor_state.syntax->types};

    EditorHighlight highlights[] = {HL_KEYWORD, HL_TYPE};

    // single-line comment start
    char *slcs = editor_state.syntax->single_line_comment_start;
    // multi-line comment start
    char *mlcs = editor_state.syntax->multi_line_comment_start;
    // multi-line comment end
    char *mlce = editor_state.syntax->multi_line_comment_end;

    // single-line comment start length
    int slcs_len = slcs ? strlen(slcs) : 0;
    // multi-line comment start length
    int mlcs_len = mlcs ? strlen(mlcs) : 0;
    // multi-line comment end length
    int mlce_len = mlce ? strlen(mlce) : 0;

    bool prev_sep = true; // beginning of line treated as separator

    bool in_str = false;
    char str_ch = '\0'; // will be ', " or perhaps something else

    // in multi-line comment
    bool in_ml_comment =
        row->index > 0 && editor_state.rows[row->index - 1].hl_open_comment;

    int i = 0;
    while (i < row->render_size) {
        char ch = row->render[i];
        unsigned char prev_hl = i > 0 ? row->highlight[i - 1] : HL_NORMAL;

        // single-line comment check
        if (slcs_len > 0 && !in_str && !in_ml_comment) {
            if (strncmp(&row->render[i], slcs, slcs_len) == 0) {
                memset(&row->highlight[i], HL_SL_COMMENT, row->render_size - i);
                break;
            }
        }

        // multi-line comment check
        if (mlcs_len > 0 && mlce_len > 0 && !in_str) {
            if (in_ml_comment) {
                row->highlight[i] = HL_ML_COMMENT;

                // if at end
                if (strncmp(&row->render[i], mlce, mlce_len) == 0) {
                    memset(&row->highlight[i], HL_ML_COMMENT, mlce_len);
                    i += mlce_len;
                    in_ml_comment = false;
                    prev_sep = true;
                    goto continue_iteration;
                } else {
                    i++;
                    goto continue_iteration;
                }
            }
            // if at start
            else if (strncmp(&row->render[i], mlcs, mlcs_len) == 0) {
                memset(&row->highlight[i], HL_ML_COMMENT, mlcs_len);
                i += mlcs_len;
                in_ml_comment = true;
                goto continue_iteration;
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
                    goto continue_iteration;
                }

                if (ch == str_ch) { in_str = false; }
                i++;
                goto continue_iteration;

            } else {
                if (ch == '"' || ch == '\'') {
                    in_str = true;
                    str_ch = ch;
                    row->highlight[i] = HL_STRING;
                    i++;
                    goto continue_iteration;
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
                goto continue_iteration;
            }
        }

        // highlight words (keywords, types etc.)
        for (int j = 0; j < (int)ARRAY_LEN(highlight_words); j++) {
            if (prev_sep) {
                for (int k = 0; highlight_words[j][k]; k++) {
                    int word_len = strlen(highlight_words[j][k]);
                    // if space for keyword
                    if (i + word_len < row->render_size) {
                        if (strncmp(&row->render[i], highlight_words[j][k],
                                    word_len) == 0) {
                            // if keyword followed by separator
                            if (is_separator(row->render[i + word_len])) {
                                memset(&row->highlight[i], highlights[j],
                                       word_len);
                                i += word_len;
                                prev_sep = true;
                                goto continue_iteration;
                            }
                        }
                    }
                }
            }
        }

        prev_sep = is_separator(ch);
        i++;

    continue_iteration:
        continue;
    }

    // update following rows when multiline comment is updated
    bool changed = row->hl_open_comment != in_ml_comment;
    row->hl_open_comment = in_ml_comment;
    if (changed && row->index + 1 < editor_state.num_rows) {
        editor_update_syntax_highlight(&editor_state.rows[row->index + 1]);
    }
}

void editor_update_syntax_highlight_all(void) {
    for (int row_index = 0; row_index < editor_state.num_rows; row_index++) {
        editor_update_syntax_highlight(&editor_state.rows[row_index]);
    }
}

void editor_apply_find_mode_highlights(void) {
    int match_len = strlen(editor_state.find_state.string);

    for (int match_index = 0;
         match_index < editor_state.find_state.matches_count; match_index++) {
        FindMatch *match = &editor_state.find_state.matches[match_index];
        EditorRow *row = &editor_state.rows[match->row];
        memset(&row->highlight[match->col], HL_MATCH, match_len);
    }
}

// returns code(s) to be inserted into a select graphic rendition sequence
// https://en.wikipedia.org/wiki/ANSI_escape_code#Select_Graphic_Rendition_parameters
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
    case HL_SL_COMMENT:
        return "39;2"; // default fg color, dim
    case HL_ML_COMMENT:
        return "39;2";
    case HL_MATCH:
        return "30;44;1"; // black fg, blue bg, bold
    default:
        return "39"; // default fg color
    }
}

void editor_set_syntax_highlight(char *file_name) {
    editor_state.syntax = NULL;
    if (!file_name) { return; }

    // get last occurence of char
    char *extension = strrchr(file_name, '.');

    int i = 0;
    while (true) {
        EditorSyntax *syntax = syntax_db[i];

        if (!syntax) { return; }

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

        i++;
    }
}
