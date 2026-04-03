#include "moon_view.h"
#include "../modules/sun_tracker_module.h"
#include "../utilities/weather.h"
#include "../modules/step_tracker_module.h" // for STEP_TRACK_WIDTH, STEP_TRACK_MARGIN
#include "../utilities/date_format.h"
#include "../modules/outer_ring_module.h"

static Window *s_moon_window = NULL;
static TextLayer *s_sunrise_text_layer = NULL;
static TextLayer *s_sunset_text_layer = NULL;
static Layer *s_sun_canvas_layer = NULL;
static GBitmap *bitmap_moon_background = NULL;
static GBitmap *bitmap_moon_phase = NULL;
static BitmapLayer *s_bg_bitmap_layer = NULL;
static BitmapLayer *s_phase_bitmap_layer = NULL;

// Moon phase resource IDs indexed by moon_phase_icon (1-7)
static const uint32_t s_moon_phase_resources[] = {
  0, // index 0 unused
  RESOURCE_ID_MOON_PHASE_1_IMAGE,
  RESOURCE_ID_MOON_PHASE_2_IMAGE,
  RESOURCE_ID_MOON_PHASE_3_IMAGE,
  RESOURCE_ID_MOON_PHASE_4_IMAGE,
  RESOURCE_ID_MOON_PHASE_5_IMAGE,
  RESOURCE_ID_MOON_PHASE_6_IMAGE,
  RESOURCE_ID_MOON_PHASE_7_IMAGE,
};

static void sun_canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Draw outer ring with hour numbers and tickers
  outer_ring_draw(ctx, bounds);
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  outer_ring_draw_tickers(ctx, bounds, t->tm_hour, t->tm_min, t->tm_sec);
  outer_ring_draw_numbers(ctx, bounds);

  int radius, diameter;
  GPoint center;
  GRect arc_bounds;

  const int BASE_RECT_WIDTH = 174;
  const int BASE_RECT_HEIGHT = 190;

  int x_offset = (bounds.size.w - BASE_RECT_WIDTH) / 2 + bounds.origin.x;
  int y_offset = (bounds.size.h - BASE_RECT_HEIGHT) / 2 + bounds.origin.y;
  radius = BASE_RECT_WIDTH / 2 + STEP_TRACK_MARGIN;
  diameter = radius * 2;
  center = GPoint(x_offset + BASE_RECT_WIDTH / 2, y_offset + BASE_RECT_HEIGHT + STEP_TRACK_MARGIN - 3);
  arc_bounds = GRect(center.x - radius, y_offset + 7, diameter, diameter);
  
  sun_tracker_module_draw(layer, ctx, bounds, radius, arc_bounds);
}

static BitmapLayer *create_centered_bitmap_layer(Layer *parent, GBitmap *bitmap, GRect bounds) {
  GRect bitmap_bounds = gbitmap_get_bounds(bitmap);
  int x = (bounds.size.w - bitmap_bounds.size.w) / 2 + 1;
  int y = (bounds.size.h - bitmap_bounds.size.h) / 2 + 7;
  BitmapLayer *layer = bitmap_layer_create(GRect(x, y, bitmap_bounds.size.w, bitmap_bounds.size.h));
  if (layer) {
    bitmap_layer_set_bitmap(layer, bitmap);
    bitmap_layer_set_compositing_mode(layer, GCompOpSet);
    layer_add_child(parent, bitmap_layer_get_layer(layer));
  }
  return layer;
}

static void moon_view_timer_callback(void *data) {
  moon_view_module_hide();
}

static TextLayer *create_info_text_layer(Layer *parent, GRect frame, const char *text) {
  TextLayer *layer = text_layer_create(frame);
  text_layer_set_background_color(layer, GColorClear);
  text_layer_set_text_color(layer, GColorWhite);
  text_layer_set_font(layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(layer, GTextAlignmentCenter);
  text_layer_set_text(layer, text);
  layer_add_child(parent, text_layer_get_layer(layer));
  return layer;
}

static void moon_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  WeatherData *weather = weather_module_get_data();
  bool has_weather = weather && weather->is_valid;

  window_set_background_color(window, GColorBlack);

  // Load moon bitmaps
  bitmap_moon_background = gbitmap_create_with_resource(RESOURCE_ID_MOON_BACKGROUND_IMAGE);

  if (has_weather && weather->moon_phase_icon >= 1 && weather->moon_phase_icon <= 7) {
    bitmap_moon_phase = gbitmap_create_with_resource(s_moon_phase_resources[weather->moon_phase_icon]);
  }

  // Sun tracker bar
  s_sun_canvas_layer = layer_create(bounds);
  if (s_sun_canvas_layer) {
    layer_set_update_proc(s_sun_canvas_layer, sun_canvas_update_proc);
    layer_add_child(window_layer, s_sun_canvas_layer);
  }

  sun_tracker_module_init(window, bounds);
  sun_tracker_module_update();

  // Sunrise / sunset text
  static char sunrise_buf[40];
  static char sunset_buf[40];
  snprintf(sunrise_buf, sizeof(sunrise_buf), "SUNRISE: --:--");
  snprintf(sunset_buf, sizeof(sunset_buf), "SUNSET: --:--");
  if (has_weather) {
    const char *fmt_24 = clock_is_24h_style() ? "%H:%M" : "%I:%M";

    struct tm t_sunrise, t_sunset;
    if (from_string_to_tm(weather->sunrise, &t_sunrise)) {
      snprintf(sunrise_buf, sizeof(sunrise_buf), "SUNRISE: ");
      strftime(sunrise_buf + 9, sizeof(sunrise_buf) - 9, fmt_24, &t_sunrise);
    }

    if (from_string_to_tm(weather->sunset, &t_sunset)) {
      snprintf(sunset_buf, sizeof(sunset_buf), "SUNSET: ");
      strftime(sunset_buf + 8, sizeof(sunset_buf) - 8, fmt_24, &t_sunset);
    }
  }

  s_sunrise_text_layer = create_info_text_layer(window_layer, GRect(0, 45, bounds.size.w, 50), sunrise_buf);
  s_sunset_text_layer = create_info_text_layer(window_layer, GRect(0, 65, bounds.size.w, 50), sunset_buf);

  // Moon phase bitmaps
  s_bg_bitmap_layer = create_centered_bitmap_layer(window_layer, bitmap_moon_background, bounds);
  if (bitmap_moon_phase) {
    s_phase_bitmap_layer = create_centered_bitmap_layer(window_layer, bitmap_moon_phase, bounds);
  }

  // Auto-dismiss after 5 seconds
  app_timer_register(MOON_VIEW_DURATION_MS, moon_view_timer_callback, NULL);
}

static void moon_window_unload(Window *window) {
  sun_tracker_module_deinit();

  if (s_phase_bitmap_layer) {
    bitmap_layer_destroy(s_phase_bitmap_layer);
    s_phase_bitmap_layer = NULL;
  }
  if (s_bg_bitmap_layer) {
    bitmap_layer_destroy(s_bg_bitmap_layer);
    s_bg_bitmap_layer = NULL;
  }
  if (bitmap_moon_phase) {
    gbitmap_destroy(bitmap_moon_phase);
    bitmap_moon_phase = NULL;
  }
  if (bitmap_moon_background) {
    gbitmap_destroy(bitmap_moon_background);
    bitmap_moon_background = NULL;
  }
  if (s_sun_canvas_layer) {
    layer_destroy(s_sun_canvas_layer);
    s_sun_canvas_layer = NULL;
  }
  if (s_sunrise_text_layer) {
    text_layer_destroy(s_sunrise_text_layer);
    s_sunrise_text_layer = NULL;
  }
  if (s_sunset_text_layer) {
    text_layer_destroy(s_sunset_text_layer);
    s_sunset_text_layer = NULL;
  }
}

void moon_view_module_init(void) {
  if (!s_moon_window) {
    s_moon_window = window_create();
    window_set_window_handlers(s_moon_window, (WindowHandlers) {
      .load = moon_window_load,
      .unload = moon_window_unload,
    });
  }
}

void moon_view_module_deinit(void) {
  if (s_moon_window) {
    window_destroy(s_moon_window);
    s_moon_window = NULL;
  }
}

void moon_view_module_show(void) {
  if (s_moon_window) {
    window_stack_push(s_moon_window, true);
  }
}

void moon_view_module_hide(void) {
  if (s_moon_window) {
    window_stack_remove(s_moon_window, true);
  }
}
