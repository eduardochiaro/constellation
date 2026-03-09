#pragma once
#include <pebble.h>

// Initializes the splash logo module
void splash_logo_init(void);

// Displays the splash logo (style: 1-7, 0=none)
void splash_logo_show(Window *window, int style);

// Cleans up resources used by the splash logo module
void splash_logo_cleanup(void);
