#include "config.h"

#include "a1.h"

void command_mode_entry(void *data) {}

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

void command_mode_exit(void) {}
