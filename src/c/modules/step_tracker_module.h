#pragma once
#include <pebble.h>

// Step Tracker Module - Step Progress Arc and Icons

#define STEP_TRACK_WIDTH 15
#define STEP_TRACK_MARGIN 4
#define WALKING_ICON_SIZE 15

void step_tracker_module_init(Window *window, GRect bounds, Layer *canvas_layer);
void step_tracker_module_update(void);
void step_tracker_module_deinit(void);
void step_tracker_module_subscribe(void);
void step_tracker_module_unsubscribe(void);
void step_tracker_module_set_goal(int goal);
int step_tracker_module_get_count(void);
void step_tracker_module_draw(Layer *layer, GContext *ctx, GRect bounds, int radius, GRect arc_bounds, bool use_line_style);
