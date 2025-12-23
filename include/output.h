#pragma once

#include "a1.h"

typedef enum { MSG_INFO, MSG_WARNING, MSG_ERROR } StatusMessageType;

void editor_draw_welcome_text(void);
void editor_refresh_screen(void);
void editor_page_scroll(EditorDirection dir, bool half);
void editor_set_status_message(StatusMessageType msg_type, const char *fmt,
                               ...);
