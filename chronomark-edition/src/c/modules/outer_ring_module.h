#pragma once
#include <pebble.h>

// Draws the Gabbro outer decorative ring (two circles)
void outer_ring_draw(GContext *ctx, GRect bounds);

// Draws the 12, 3, 6, 9 hour numbers between the rings
void outer_ring_draw_numbers(GContext *ctx, GRect bounds);

// Draws hour, minute, and second ticker hands (second only if show_seconds is true)
void outer_ring_draw_tickers(GContext *ctx, GRect bounds, int hour, int minute, int second, bool show_seconds);
