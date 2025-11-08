#pragma once

#include "a1.h"
#include "structures.h"

void editor_refresh_screen(void);
void editor_page_scroll(EditorDirection dir, bool half);
void editor_scroll_render_update(void);
void editor_draw_rows(AppendBuffer *ab);
void editor_draw_status_bar(AppendBuffer *ab);
void editor_draw_message_bar(AppendBuffer *ab);
void editor_draw_welcome_text(void);
void editor_set_status_message(const char *fmt, ...);
