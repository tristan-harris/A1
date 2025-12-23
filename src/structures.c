#include "structures.h"
#include "terminal.h"
#include <stdlib.h>
#include <string.h>

void ab_append(AppendBuffer *ab, const char *string, int len) {
    if (len == 0) { return; }

    char *new = realloc(ab->buf, ab->len + len);
    if (new == NULL) { terminal_die("realloc"); }

    memcpy(&new[ab->len], string, len);
    ab->buf = new;
    ab->len += len;
}

void ab_free(AppendBuffer *ab) {
    free(ab->buf);
}
