#pragma once
#include <pebble.h>
#include "date_format.h"

// Bottom Module - Configurable Date Display

void bottom_module_init(Window *window, GRect bounds);
void bottom_module_update(struct tm *tick_time, DateFormatType format, int step_count);
void bottom_module_deinit(void);
