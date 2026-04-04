#pragma once
#include <pebble.h>

void weather_display_module_init(Window *window, GRect bounds, int weather_y_offset);
void weather_display_module_update(void);
void weather_display_module_set_visible(bool visible);
void weather_display_module_deinit(void);
