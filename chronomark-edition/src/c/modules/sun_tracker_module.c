#include "sun_tracker_module.h"
#include "../utilities/weather.h"
#include <string.h>
#include <stdlib.h>

static BitmapLayer *s_left_icon_layer = NULL;
static BitmapLayer *s_right_icon_layer = NULL;
static GBitmap *s_left_bitmap = NULL;
static GBitmap *s_right_bitmap = NULL;

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

  s_left_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SUN_UP_IMAGE);
  s_right_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SUN_DOWN_IMAGE);

  // Compute arc geometry (mirrors moon_view sun_canvas_update_proc)
  int radius = 175 / 2 + STEP_TRACK_MARGIN;
  int cy = bounds.size.h / 2;

  // Left icon at 270° (top of arc)
  if (s_left_bitmap) {
    int icon_x = 0 + (bounds.size.w / 2 - radius);
    int icon_y = cy - WALKING_ICON_SIZE;
    s_left_icon_layer = bitmap_layer_create(GRect(icon_x, icon_y, WALKING_ICON_SIZE, WALKING_ICON_SIZE));
    if (s_left_icon_layer) {
      bitmap_layer_set_bitmap(s_left_icon_layer, s_left_bitmap);
      bitmap_layer_set_compositing_mode(s_left_icon_layer, GCompOpSet);
      layer_add_child(window_layer, bitmap_layer_get_layer(s_left_icon_layer));
    }
  }

  // Right icon at 90° (bottom of arc)
  if (s_right_bitmap) {
    int icon_x = bounds.size.w - (bounds.size.w / 2 - radius) - WALKING_ICON_SIZE;
    int icon_y = cy - WALKING_ICON_SIZE;
    s_right_icon_layer = bitmap_layer_create(GRect(icon_x, icon_y, WALKING_ICON_SIZE, WALKING_ICON_SIZE));
    if (s_right_icon_layer) {
      bitmap_layer_set_bitmap(s_right_icon_layer, s_right_bitmap);
      bitmap_layer_set_compositing_mode(s_right_icon_layer, GCompOpSet);
      layer_add_child(window_layer, bitmap_layer_get_layer(s_right_icon_layer));
    }
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
    if (s_left_icon_layer && s_right_bitmap) {
      bitmap_layer_set_bitmap(s_left_icon_layer, s_right_bitmap);
    }
    if (s_right_icon_layer && s_left_bitmap) {
      bitmap_layer_set_bitmap(s_right_icon_layer, s_left_bitmap);
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
    if (s_left_icon_layer && s_right_bitmap)
      bitmap_layer_set_bitmap(s_left_icon_layer, s_right_bitmap);
    if (s_right_icon_layer && s_left_bitmap)
      bitmap_layer_set_bitmap(s_right_icon_layer, s_left_bitmap);
  } else {
    if (s_left_icon_layer && s_left_bitmap)
      bitmap_layer_set_bitmap(s_left_icon_layer, s_left_bitmap);
    if (s_right_icon_layer && s_right_bitmap)
      bitmap_layer_set_bitmap(s_right_icon_layer, s_right_bitmap);
  }
}

void sun_tracker_module_draw(Layer *layer, GContext *ctx, GRect bounds, int radius, GRect arc_bounds) {
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

void sun_tracker_module_deinit(void) {
  if (s_left_icon_layer) {
    bitmap_layer_destroy(s_left_icon_layer);
    s_left_icon_layer = NULL;
  }
  if (s_right_icon_layer) {
    bitmap_layer_destroy(s_right_icon_layer);
    s_right_icon_layer = NULL;
  }
  if (s_left_bitmap) {
    gbitmap_destroy(s_left_bitmap);
    s_left_bitmap = NULL;
  }
  if (s_right_bitmap) {
    gbitmap_destroy(s_right_bitmap);
    s_right_bitmap = NULL;
  }
}
