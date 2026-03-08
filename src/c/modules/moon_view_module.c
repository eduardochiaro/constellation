#include "moon_view_module.h"
#include "sun_tracker_module.h"
#include "weather_module.h"
#include "step_tracker_module.h" // for STEP_TRACK_WIDTH, STEP_TRACK_MARGIN

static Window *s_moon_window = NULL;
static TextLayer *s_moon_text_layer = NULL;
static Layer *s_sun_canvas_layer = NULL;
static bool s_use_line_style = false;

static void sun_canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  int radius, diameter;
  GPoint center;
  GRect arc_bounds;

#if defined(PBL_ROUND)
  const int BASE_RECT_WIDTH = 150;
  const int BASE_RECT_HEIGHT = 168;
  int x_offset = (bounds.size.w - BASE_RECT_WIDTH) / 2 + bounds.origin.x;
  int y_offset = (bounds.size.h - BASE_RECT_HEIGHT) / 2 + bounds.origin.y;
  radius = BASE_RECT_WIDTH / 2 + STEP_TRACK_MARGIN;
  diameter = radius * 2;
  center = GPoint(x_offset + BASE_RECT_WIDTH / 2, y_offset + BASE_RECT_HEIGHT + STEP_TRACK_MARGIN - 3);
  arc_bounds = GRect(center.x - radius, y_offset + 7, diameter, diameter);
#else
  radius = bounds.size.w / 2 + STEP_TRACK_MARGIN;
  diameter = radius * 2 - 15;
  center = GPoint(bounds.origin.x + bounds.size.w / 2, bounds.origin.y + bounds.size.h + STEP_TRACK_MARGIN - 3);
  arc_bounds = GRect(center.x - radius + 8, bounds.origin.y + 22, diameter, diameter);
#endif

  sun_tracker_module_draw(layer, ctx, bounds, radius, arc_bounds, s_use_line_style);
}

static void moon_view_timer_callback(void *data) {
  moon_view_module_hide();
}

static void moon_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  window_set_background_color(window, GColorBlack);

  // Create canvas layer for sun tracker bar
  s_sun_canvas_layer = layer_create(bounds);
  if (s_sun_canvas_layer) {
    layer_set_update_proc(s_sun_canvas_layer, sun_canvas_update_proc);
    layer_add_child(window_layer, s_sun_canvas_layer);
  }

  // Initialize sun tracker icons on this window
  sun_tracker_module_init(window, bounds);
  sun_tracker_module_update();

  // Moon phase text
  WeatherData *weather = weather_module_get_data();
  static char moon_buf[40];
  if (weather->is_valid) {
    snprintf(moon_buf, sizeof(moon_buf), "%s\n%d%%", weather->moon_phase_name, weather->moon_phase);
  } else {
    snprintf(moon_buf, sizeof(moon_buf), "Moon View");
  }

  s_moon_text_layer = text_layer_create(GRect(0, bounds.size.h / 2 - 60, bounds.size.w, 50));
  text_layer_set_background_color(s_moon_text_layer, GColorClear);
  text_layer_set_text_color(s_moon_text_layer, GColorWhite);
  text_layer_set_font(s_moon_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_alignment(s_moon_text_layer, GTextAlignmentCenter);
  text_layer_set_text(s_moon_text_layer, moon_buf);
  layer_add_child(window_layer, text_layer_get_layer(s_moon_text_layer));

  // Auto-dismiss after 5 seconds
  app_timer_register(MOON_VIEW_DURATION_MS, moon_view_timer_callback, NULL);
}

static void moon_window_unload(Window *window) {
  sun_tracker_module_deinit();

  if (s_sun_canvas_layer) {
    layer_destroy(s_sun_canvas_layer);
    s_sun_canvas_layer = NULL;
  }
  if (s_moon_text_layer) {
    text_layer_destroy(s_moon_text_layer);
    s_moon_text_layer = NULL;
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

void moon_view_module_set_line_style(bool use_line) {
  s_use_line_style = use_line;
}
