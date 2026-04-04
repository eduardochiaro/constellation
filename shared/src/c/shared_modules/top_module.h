#pragma once
#include <pebble.h>
#include "../utilities/date_format.h"

// Top Module - Configurable Date Display

void top_module_init(Window *window, GRect bounds, int text_y_offset, uint32_t walk_icon_res, int icon_y_offset);
void top_module_update(struct tm *tick_time, DateFormatType format, int step_count, int distance_walked, bool use_miles);
void top_module_deinit(void);
