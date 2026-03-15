#pragma once
#include <pebble.h>
#include "step_tracker_module.h" // for STEP_TRACK_WIDTH, STEP_TRACK_MARGIN, WALKING_ICON_SIZE

#define SUN_ICON_SIZE 15

// Initialize the sun tracker module (creates icon layers)
void sun_tracker_module_init(Window *window, GRect bounds);

// Draw the sun tracker bar (called from a canvas update proc)
void sun_tracker_module_draw(Layer *layer, GContext *ctx, GRect bounds, int radius, GRect arc_bounds, bool use_line_style);

// Update sun tracker state (recalculates progress from current time + weather data)
void sun_tracker_module_update(void);

// Deinitialize and free resources
void sun_tracker_module_deinit(void);
