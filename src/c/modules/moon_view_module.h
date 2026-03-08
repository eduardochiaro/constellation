#pragma once
#include <pebble.h>

#define MOON_VIEW_DURATION_MS 5000

void moon_view_module_init(void);
void moon_view_module_deinit(void);
void moon_view_module_show(void);
void moon_view_module_hide(void);

// Set whether to use line style (rect) for the sun tracker bar
void moon_view_module_set_line_style(bool use_line);
