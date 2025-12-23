#include "a1.h"
#include <argp.h>
#include <stdbool.h>

// adapted from
// https://sourceware.org/glibc/manual/latest/html_node/Argp-Example-3.html

const char *argp_program_version = "v" A1_VERSION;

// program description/documentation
static char doc[] =
    "A1 - A terminal text-editor (use --manual for further information)";

// description of accepted arguments
static char args_doc[] = "[FILE]";

// empty struct at end is terminating
static struct argp_option options[] = {
    {"clean", 'c', 0, 0, "Do not apply configuration", 0},
    {"config", 'f', "FILE", 0, "Apply config from this file", 0},
    {"manual", 'm', 0, 0, "Print manual and exit", 0},
    {0}};

static error_t parse_option(int key, char *arg, struct argp_state *state) {
    // get EditorArguments pointer from the input argument from argp_parse
    EditorArguments *arguments = state->input;

    switch (key) {
    case 'c':
        arguments->clean = true;
        break;
    case 'f':
        arguments->config_file_path = arg;
        break;
    case 'm':
        arguments->manual = true;
        break;
    case ARGP_KEY_ARG:
        // if more than one (file) argument
        if (state->arg_num > 0) { argp_usage(state); }
        arguments->file_path = arg;
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

// argp parser
static struct argp argp = {options, parse_option, args_doc, doc, 0, 0, 0};

void parse_arguments(EditorArguments *arguments, int argc, char *argv[]) {
    argp_parse(&argp, argc, argv, 0, 0, arguments);
}
