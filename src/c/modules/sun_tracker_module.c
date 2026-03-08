#include "sun_tracker_module.h"
#include "weather_module.h"
#include <string.h>
#include <stdlib.h>

static BitmapLayer *s_left_icon_layer = NULL;
static BitmapLayer *s_right_icon_layer = NULL;
static GBitmap *s_sun_up_bitmap = NULL;
static GBitmap *s_sun_down_bitmap = NULL;

static float s_progress = 0.0f;
static bool s_is_daytime = true;

// Parse "2026-03-08T06:30" ISO time string into minutes since midnight
static int parse_iso_minutes(const char *iso_str) {
  if (!iso_str || strlen(iso_str) < 16) return -1;
  // Find the 'T' separator
  const char *t = strchr(iso_str, 'T');
  if (!t) return -1;
  int hours = atoi(t + 1);
  // Find the ':' after hours
  const char *colon = strchr(t + 1, ':');
  if (!colon) return -1;
  int minutes = atoi(colon + 1);
  return hours * 60 + minutes;
}

void sun_tracker_module_init(Window *window, GRect bounds) {
  Layer *window_layer = window_get_root_layer(window);

  s_sun_up_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SUN_UP_IMAGE);
  s_sun_down_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SUN_DOWN_IMAGE);

  // Left icon
  int left_x = 5;
  int left_y = bounds.size.h / 2 - SUN_ICON_SIZE + 4;
#if defined(PBL_ROUND)
  left_x += 8;
  left_y -= 3;
#endif
  s_left_icon_layer = bitmap_layer_create(GRect(left_x, left_y, SUN_ICON_SIZE, SUN_ICON_SIZE));
  if (s_left_icon_layer) {
    bitmap_layer_set_compositing_mode(s_left_icon_layer, GCompOpSet);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_left_icon_layer));
  }

  // Right icon
  int right_x = bounds.size.w - SUN_ICON_SIZE - 3;
  int right_y = bounds.size.h / 2 - SUN_ICON_SIZE + 4;
#if defined(PBL_ROUND)
  right_x -= 8;
  right_y -= 4;
#endif
  s_right_icon_layer = bitmap_layer_create(GRect(right_x, right_y, SUN_ICON_SIZE, SUN_ICON_SIZE));
  if (s_right_icon_layer) {
    bitmap_layer_set_compositing_mode(s_right_icon_layer, GCompOpSet);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_right_icon_layer));
  }

  // Initial update to set icons and progress
  sun_tracker_module_update();
}

void sun_tracker_module_update(void) {
  WeatherData *weather = weather_module_get_data();
  if (!weather->is_valid) {
    s_progress = 0.0f;
    s_is_daytime = true;
    // Set default icon arrangement (day: sun_down left, sun_up right)
    if (s_left_icon_layer && s_sun_down_bitmap) {
      bitmap_layer_set_bitmap(s_left_icon_layer, s_sun_down_bitmap);
    }
    if (s_right_icon_layer && s_sun_up_bitmap) {
      bitmap_layer_set_bitmap(s_right_icon_layer, s_sun_up_bitmap);
    }
    return;
  }

  int sunrise_min = parse_iso_minutes(weather->sunrise);
  int sunset_min = parse_iso_minutes(weather->sunset);
  if (sunrise_min < 0 || sunset_min < 0) {
    s_progress = 0.0f;
    return;
  }

  // Get current time in minutes since midnight
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int now_min = t->tm_hour * 60 + t->tm_min;

  if (now_min >= sunrise_min && now_min < sunset_min) {
    // Daytime: tracking sunrise -> sunset
    s_is_daytime = true;
    int total = sunset_min - sunrise_min;
    int elapsed = now_min - sunrise_min;
    s_progress = (total > 0) ? ((float)elapsed / (float)total) : 0.0f;
  } else {
    // Nighttime: tracking sunset -> next sunrise
    s_is_daytime = false;
    int total;
    int elapsed;
    if (now_min >= sunset_min) {
      // After sunset, before midnight
      total = (24 * 60 - sunset_min) + sunrise_min;
      elapsed = now_min - sunset_min;
    } else {
      // After midnight, before sunrise
      total = (24 * 60 - sunset_min) + sunrise_min;
      elapsed = (24 * 60 - sunset_min) + now_min;
    }
    s_progress = (total > 0) ? ((float)elapsed / (float)total) : 0.0f;
  }

  // Clamp
  if (s_progress < 0.0f) s_progress = 0.0f;
  if (s_progress > 1.0f) s_progress = 1.0f;

  // Set icons based on day/night
  if (!s_is_daytime) {
    if (s_left_icon_layer && s_sun_down_bitmap)
      bitmap_layer_set_bitmap(s_left_icon_layer, s_sun_down_bitmap);
    if (s_right_icon_layer && s_sun_up_bitmap)
      bitmap_layer_set_bitmap(s_right_icon_layer, s_sun_up_bitmap);
  } else {
    if (s_left_icon_layer && s_sun_up_bitmap)
      bitmap_layer_set_bitmap(s_left_icon_layer, s_sun_up_bitmap);
    if (s_right_icon_layer && s_sun_down_bitmap)
      bitmap_layer_set_bitmap(s_right_icon_layer, s_sun_down_bitmap);
  }
}

void sun_tracker_module_draw(Layer *layer, GContext *ctx, GRect bounds, int radius, GRect arc_bounds, bool use_line_style) {
  if (use_line_style) {
    // Line style - U-shaped perimeter along bottom and sides (same geometry as step tracker)
    const int line_width = STEP_TRACK_WIDTH;
    const int margin = 3;

    int left_x = margin;
    int right_x = bounds.size.w - margin - line_width;
    int top_y = bounds.size.h / 2 + 7;
    int bottom_y = bounds.size.h - margin - line_width;

    int left_height = bottom_y - top_y + 15;
    int bottom_width = right_x - left_x;
    int total_perimeter = left_height + bottom_width + left_height;

    int progress_distance = (int)(total_perimeter * s_progress);

    // Draw base perimeter (dark gray)
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    graphics_fill_rect(ctx, GRect(left_x, top_y, line_width, left_height), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(left_x, bottom_y, bottom_width + line_width, line_width), 0, GCornerNone);
    graphics_fill_rect(ctx, GRect(right_x, top_y, line_width, left_height), 0, GCornerNone);

    // Draw progress - fills from RIGHT to LEFT (right side down, bottom, left side up)
    if (s_progress > 0) {
      graphics_context_set_fill_color(ctx, GColorWhite);
      if (progress_distance <= left_height) {
        // Progress on left side (top to bottom)
        int fill_height = progress_distance;
        graphics_fill_rect(ctx, GRect(left_x, top_y, line_width, fill_height), 0, GCornerNone);
      } else if (progress_distance <= left_height + bottom_width) {
        graphics_fill_rect(ctx, GRect(left_x, top_y, line_width, left_height), 0, GCornerNone);
        int bottom_progress = progress_distance - left_height;
        graphics_fill_rect(ctx, GRect(left_x, bottom_y, bottom_progress, line_width), 0, GCornerNone);
      } else {
        graphics_fill_rect(ctx, GRect(left_x, top_y, line_width, left_height), 0, GCornerNone);
        graphics_fill_rect(ctx, GRect(left_x, bottom_y, bottom_width + line_width, line_width), 0, GCornerNone);
        int right_progress = progress_distance - left_height - bottom_width - 15;
        graphics_fill_rect(ctx, GRect(right_x, bottom_y - right_progress, line_width, right_progress), 0, GCornerNone);
      }
    }
  } else {
    // Arc style - curved design (mirrors the step tracker arc)
    // Step tracker uses 90° to 270° (left arc). Sun tracker uses 270° to 90° (right arc).
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    graphics_fill_radial(ctx, arc_bounds, GOvalScaleModeFitCircle, STEP_TRACK_WIDTH,
                         DEG_TO_TRIGANGLE(90), DEG_TO_TRIGANGLE(270));

    if (s_progress > 0) {
      // Fill from top (270°) clockwise toward bottom (450°/90°)
      int32_t start_angle = 270 - (int32_t)(180.0f * s_progress);
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_fill_radial(ctx, arc_bounds, GOvalScaleModeFitCircle, STEP_TRACK_WIDTH,
                           DEG_TO_TRIGANGLE(start_angle), DEG_TO_TRIGANGLE(270));
    }
  }
}

void sun_tracker_module_deinit(void) {
  if (s_left_icon_layer) {
    bitmap_layer_destroy(s_left_icon_layer);
    s_left_icon_layer = NULL;
  }
  if (s_right_icon_layer) {
    bitmap_layer_destroy(s_right_icon_layer);
    s_right_icon_layer = NULL;
  }
  if (s_sun_up_bitmap) {
    gbitmap_destroy(s_sun_up_bitmap);
    s_sun_up_bitmap = NULL;
  }
  if (s_sun_down_bitmap) {
    gbitmap_destroy(s_sun_down_bitmap);
    s_sun_down_bitmap = NULL;
  }
}
