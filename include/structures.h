#ifndef STRUCTURES_H
#define STRUCTURES_H

// A very simple "append buffer" structure, that is a heap
// allocated string that can be appended to. This is useful in order to
// write all the escape sequences in a buffer and flush them to the standard
// output in a single call, to avoid flickering effects.

typedef struct {
    char *buf;
    int len;
} AppendBuffer;

void ab_append(AppendBuffer *ab, const char *string, int len);
void ab_free(AppendBuffer *ab);

#endif
