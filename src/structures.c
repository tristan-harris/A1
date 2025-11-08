#include "config.h"

#include "structures.h"

#include <stdlib.h>
#include <string.h>

void ab_append(AppendBuffer *ab, const char *string, int len) {
    char *new = realloc(ab->buf, ab->len + len);

    if (new == NULL) {
        return;
    }
    memcpy(&new[ab->len], string, len);
    ab->buf = new;
    ab->len += len;
}

void ab_free(AppendBuffer *ab) { free(ab->buf); }
