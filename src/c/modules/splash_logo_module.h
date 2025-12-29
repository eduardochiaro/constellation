#pragma once
#include <pebble.h>

// Initializes the splash logo module
void splash_logo_init(void);

// Displays the splash logo
void splash_logo_show(Window *window, char *s_style_logo);

// Hides the splash logo
void splash_logo_hide(void);

// Cleans up resources used by the splash logo module
void splash_logo_cleanup(void);
