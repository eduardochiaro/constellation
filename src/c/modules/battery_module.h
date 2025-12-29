#pragma once
#include <pebble.h>

// Battery Module - Battery Indicator Display

#define BATTERY_WIDTH 25
#define BATTERY_HEIGHT 2

void battery_module_init(Window *window, GRect bounds);
void battery_module_update(void);
void battery_module_deinit(void);
void battery_module_subscribe(void);
void battery_module_unsubscribe(void);
