#include "moon_view_module.h"
#include "sun_tracker_module.h"
#include "weather_module.h"
#include "step_tracker_module.h" // for STEP_TRACK_WIDTH, STEP_TRACK_MARGIN

static Window *s_moon_window = NULL;
static TextLayer *s_moon_text_layer = NULL;
static Layer *s_sun_canvas_layer = NULL;
static bool s_use_line_style = false;
static GBitmap *bitmap_moon_background = NULL;
static GBitmap *bitmap_moon_phase = NULL;

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

static void set_moon_layer_bitmap(Layer *window_layer, GBitmap *bitmap, GRect bounds) {

   GRect bitmap_bounds = gbitmap_get_bounds(bitmap);
    int x = (bounds.size.w - bitmap_bounds.size.w) / 2 + 1;
    int y = (bounds.size.h - bitmap_bounds.size.h) / 2 + 7;
    BitmapLayer *bitmap_layer = bitmap_layer_create(GRect(x, y, bitmap_bounds.size.w, bitmap_bounds.size.h));
    if (bitmap_layer) {
      bitmap_layer_set_bitmap(bitmap_layer, bitmap);
      bitmap_layer_set_compositing_mode(bitmap_layer, GCompOpSet);
      layer_add_child(window_layer, bitmap_layer_get_layer(bitmap_layer));
    }
}

static void moon_view_timer_callback(void *data) {
  moon_view_module_hide();
}

static struct tm *from_string_to_tm(const char *time_str) {
  // Parse "2026-03-08T06:30" manually (no sscanf to avoid pulling in libc)
  static struct tm t;
  memset(&t, 0, sizeof(struct tm));
  if (!time_str || strlen(time_str) < 16) return NULL;

  t.tm_year = atoi(time_str) - 1900;       // "2026" -> 126
  t.tm_mon  = atoi(time_str + 5) - 1;      // "03"   -> 2
  t.tm_mday = atoi(time_str + 8);           // "08"
  t.tm_hour = atoi(time_str + 11);          // "06"
  t.tm_min  = atoi(time_str + 14);          // "30"
  return &t;
}

static void moon_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  // get weather data
  WeatherData *weather = weather_module_get_data();

  bool show_moon_phase = true;

  window_set_background_color(window, GColorBlack);

  bitmap_moon_background = gbitmap_create_with_resource(RESOURCE_ID_MOON_BACKGROUND_IMAGE);
  //get moon phase icon based on moon icon
   if (weather->is_valid) {
     // Map moon phase icon index to resource ID
    APP_LOG(APP_LOG_LEVEL_INFO, "moon icon: %d", weather->moon_phase_icon);
    switch (weather->moon_phase_icon)
    {
      case 1:
        bitmap_moon_phase = gbitmap_create_with_resource(RESOURCE_ID_MOON_PHASE_1_IMAGE);
        break;
      case 2:
        bitmap_moon_phase = gbitmap_create_with_resource(RESOURCE_ID_MOON_PHASE_2_IMAGE);
        break;
      case 3:
        bitmap_moon_phase = gbitmap_create_with_resource(RESOURCE_ID_MOON_PHASE_3_IMAGE);
        break;
      case 4:
        bitmap_moon_phase = gbitmap_create_with_resource(RESOURCE_ID_MOON_PHASE_4_IMAGE);
        break;
      case 5:
        bitmap_moon_phase = gbitmap_create_with_resource(RESOURCE_ID_MOON_PHASE_5_IMAGE);
        break;
      case 6:
        bitmap_moon_phase = gbitmap_create_with_resource(RESOURCE_ID_MOON_PHASE_6_IMAGE);
        break;
      case 7:
        bitmap_moon_phase = gbitmap_create_with_resource(RESOURCE_ID_MOON_PHASE_7_IMAGE);
        break;
      default:
        show_moon_phase = false;
        break;
     }
   } else {
    show_moon_phase = false;
   }

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
  static char sunrise_buf[40];
  static char sunset_buf[40];
  if (weather->is_valid) {

    struct tm *t_sunrise= from_string_to_tm(weather->sunrise);
    strftime(sunrise_buf, sizeof(sunrise_buf), clock_is_24h_style() ? "SUNRISE: %H:%M" : "SUNRISE: %I:%M", t_sunrise);

    struct tm *t_sunset= from_string_to_tm(weather->sunset);
    strftime(sunset_buf, sizeof(sunset_buf), clock_is_24h_style() ? "SUNSET: %H:%M" : "SUNSET: %I:%M", t_sunset);

    //snprintf(moon_buf, sizeof(moon_buf), "%s\n%d%%", weather->moon_phase_name, weather->moon_phase);
  }

  s_moon_text_layer = text_layer_create(GRect(0, 5, bounds.size.w, 50));
  text_layer_set_background_color(s_moon_text_layer, GColorClear);
  text_layer_set_text_color(s_moon_text_layer, GColorWhite);
  text_layer_set_font(s_moon_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_moon_text_layer, GTextAlignmentCenter);
  text_layer_set_text(s_moon_text_layer, sunrise_buf);
  layer_add_child(window_layer, text_layer_get_layer(s_moon_text_layer));

  s_moon_text_layer = text_layer_create(GRect(0, 20, bounds.size.w, 50));
  text_layer_set_background_color(s_moon_text_layer, GColorClear);
  text_layer_set_text_color(s_moon_text_layer, GColorWhite);
  text_layer_set_font(s_moon_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD));
  text_layer_set_text_alignment(s_moon_text_layer, GTextAlignmentCenter);
  text_layer_set_text(s_moon_text_layer, sunset_buf);
  layer_add_child(window_layer, text_layer_get_layer(s_moon_text_layer));

  set_moon_layer_bitmap(window_layer, bitmap_moon_background, bounds);
  if (show_moon_phase) {
    set_moon_layer_bitmap(window_layer, bitmap_moon_phase, bounds);
  }

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
    if (bitmap_moon_phase) {
      gbitmap_destroy(bitmap_moon_phase);
      bitmap_moon_phase = NULL;
    }
    if (bitmap_moon_background) {
      gbitmap_destroy(bitmap_moon_background);
      bitmap_moon_background = NULL;
    }
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
