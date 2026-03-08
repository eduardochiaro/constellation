#pragma once
#include <pebble.h>

void weather_display_module_init(Window *window, GRect bounds);
void weather_display_module_update(void);
void weather_display_module_deinit(void);
