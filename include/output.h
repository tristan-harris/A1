#pragma once

#include "a1.h"
#include "structures.h"

typedef enum { MSG_INFO, MSG_WARNING, MSG_ERROR } StatusMessageType;

void editor_refresh_screen(void);
void editor_page_scroll(EditorDirection dir, bool half);
void editor_scroll_render_update(void);
void editor_draw_rows(AppendBuffer *ab);
void editor_add_to_status_bar_buffer(char buffer[], size_t max_len,
                                     int *buffer_len, int *buffer_render_len,
                                     const char *fmt, ...);
void editor_draw_status_bar(AppendBuffer *ab);
void editor_draw_bottom_bar(AppendBuffer *ab);
void editor_draw_welcome_text(void);
void editor_clear_status_message(void);
void editor_set_status_message(StatusMessageType msg_type, const char *fmt,
                               ...);
