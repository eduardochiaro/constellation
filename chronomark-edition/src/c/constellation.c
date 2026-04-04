#include <pebble.h>
#include "views/moon_view.h"
#include "utilities/weather.h"
#include "modules/top_module.h"
#include "modules/bottom_module.h"
#include "modules/battery_module.h"
#include "modules/step_tracker_module.h"
#include "modules/splash_logo_module.h"
#include "modules/weather_display_module.h"
#include "modules/outer_ring_module.h"

// ============================================================================
// CONSTANTS
// ============================================================================

#define SECONDS_INDICATOR_SIZE 4
#define SPLASH_DURATION_MS 2000

// ============================================================================
// GLOBAL STATE - Windows and Layers
// ============================================================================

static Window *s_window;

// Text layers (central elements only)
static TextLayer *s_time_layer;

// Custom drawing layers
static Layer *s_canvas_layer;
static Layer *s_ampm_layer;

// ============================================================================
// GLOBAL STATE - Bitmaps
// ============================================================================

static GBitmap *s_am_active_bitmap;
static GBitmap *s_am_inactive_bitmap;
static GBitmap *s_pm_active_bitmap;
static GBitmap *s_pm_inactive_bitmap;

// ============================================================================
// GLOBAL STATE - App Data
// ============================================================================

static bool s_is_pm;
static int s_current_second;
static int s_current_minute;
static int s_current_hour;
static int s_last_weather_minute = -1;
static Layer *s_clock_ring_layer;

// User settings (persisted)
static bool s_show_clock_analog = true;
static bool s_show_second_ticker = false;
static bool s_show_decorative_ring = true;
static bool s_show_step_tracker = true;
static DateFormatType s_top_module_format = DATE_FORMAT_WEEKDAY;
static DateFormatType s_bottom_module_format = DATE_FORMAT_MONTH_DAY;
static int s_step_goal = 8000;
static int s_style_logo = 1;
static bool s_tracker_use_line = false;
static bool s_show_moon_view = true;
static bool s_show_weather = true;
static int s_weather_scale = 1;
static bool s_use_miles = false;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

static void load_watchface_ui(void *data);
static DateFormatType parse_date_format(const char *format_str);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static DateFormatType parse_date_format(const char *format_str) {
  if (!format_str) return DATE_FORMAT_WEEKDAY;
  
  if (strcmp(format_str, "weekday") == 0) return DATE_FORMAT_WEEKDAY;
  if (strcmp(format_str, "month_day") == 0) return DATE_FORMAT_MONTH_DAY;
  if (strcmp(format_str, "yyyy_mm_dd") == 0) return DATE_FORMAT_YYYY_MM_DD;
  if (strcmp(format_str, "dd_mm_yyyy") == 0) return DATE_FORMAT_DD_MM_YYYY;
  if (strcmp(format_str, "mm_dd_yyyy") == 0) return DATE_FORMAT_MM_DD_YYYY;
  if (strcmp(format_str, "month_year") == 0) return DATE_FORMAT_MONTH_YEAR;
  if (strcmp(format_str, "weekday_day") == 0) return DATE_FORMAT_WEEKDAY_DAY;
  if (strcmp(format_str, "step_count") == 0) return DATE_FORMAT_STEP_COUNT;
  if (strcmp(format_str, "distance") == 0) return DATE_FORMAT_DISTANCE;
  
  return DATE_FORMAT_WEEKDAY; // Default fallback
}

static bool check_if_24h() {
  // Check if the system is set to 24h format
  return false;
  return clock_is_24h_style();
}

static void draw_bitmap_centered(GContext *ctx, GBitmap *bmp, GRect rect) {
  if (!bmp) return;
  
  GRect bmp_bounds = gbitmap_get_bounds(bmp);
  int x = rect.origin.x + (rect.size.w - bmp_bounds.size.w) / 2;
  int y = rect.origin.y + (rect.size.h - bmp_bounds.size.h) / 2;
  graphics_draw_bitmap_in_rect(ctx, bmp, GRect(x, y, bmp_bounds.size.w, bmp_bounds.size.h));
}

// ============================================================================
// LAYER UPDATE PROCEDURES
// ============================================================================

static void ampm_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int section_height = (bounds.size.h - 2) / 2;
  int width = bounds.size.w;

  GRect am_rect = GRect(0, 0, width, section_height);
  GRect pm_rect = GRect(0, section_height + 2, width, section_height);

  draw_bitmap_centered(ctx, s_is_pm ? s_am_inactive_bitmap : s_am_active_bitmap, am_rect);
  draw_bitmap_centered(ctx, s_is_pm ? s_pm_active_bitmap : s_pm_inactive_bitmap, pm_rect);
}


// Draws the static clock ring and Gabbro outer ring on their own layer (only redrawn when settings change)
static void clock_ring_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  if (s_show_clock_analog) {
    outer_ring_draw(ctx, bounds);
  }

  if (!s_show_decorative_ring) return;
  int radius;

  int BASE_RECT_WIDTH = 174;
  radius = BASE_RECT_WIDTH / 2 + STEP_TRACK_MARGIN;

  GPoint ring_center = grect_center_point(&bounds);
  int ring_outer_radius = radius - STEP_TRACK_WIDTH - 7;
  if (!s_show_step_tracker) {
    ring_outer_radius += 10;
  }
  int tick_length_normal = 2;
  int tick_length_major = 3;
  graphics_context_set_stroke_color(ctx, PBL_IF_ROUND_ELSE(GColorWhite, GColorDarkGray));
  for (int i = 0; i < 60; i++) {
    int32_t angle = (TRIG_MAX_ANGLE * i) / 60;
    bool is_major = (i % 5 == 0);
    int tick_length = is_major ? tick_length_major : tick_length_normal;
    int ring_inner_radius = ring_outer_radius - tick_length;
    GPoint outer = GPoint(
      ring_center.x + (int16_t)((sin_lookup(angle) * ring_outer_radius) / TRIG_MAX_RATIO),
      ring_center.y - (int16_t)((cos_lookup(angle) * ring_outer_radius) / TRIG_MAX_RATIO)
    );
    GPoint inner = GPoint(
      ring_center.x + (int16_t)((sin_lookup(angle) * ring_inner_radius) / TRIG_MAX_RATIO),
      ring_center.y - (int16_t)((cos_lookup(angle) * ring_inner_radius) / TRIG_MAX_RATIO)
    );
    graphics_context_set_stroke_width(ctx, is_major ? 2 : 1);
    graphics_draw_line(ctx, outer, inner);
  }
}

// Draws an outer decorative ring with hour numbers (Gabbro only)
static void draw_gabbro_outer_ring_numbers(GContext *ctx, GRect bounds) {
  outer_ring_draw_numbers(ctx, bounds);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Gabbro outer ring and clock ring are on their own static layer now

  // Calculate arc dimensions for step tracker based on platform
  int radius, diameter;
  GPoint center;
  GRect arc_bounds;

  // Round watch layout
  int BASE_RECT_WIDTH = 174;
  int BASE_RECT_HEIGHT = 190;

  int x_offset = (bounds.size.w - BASE_RECT_WIDTH) / 2 + bounds.origin.x;
  int y_offset = (bounds.size.h - BASE_RECT_HEIGHT) / 2 + bounds.origin.y;

  radius = BASE_RECT_WIDTH / 2 + STEP_TRACK_MARGIN;
  diameter = radius * 2;
  center = GPoint(x_offset + BASE_RECT_WIDTH / 2, y_offset + BASE_RECT_HEIGHT + STEP_TRACK_MARGIN - 3);
  arc_bounds = GRect(center.x - radius, y_offset + 7, diameter, diameter);

  // Draw step tracker (delegated to module)
  if (s_show_step_tracker) {
    step_tracker_module_draw(layer, ctx, bounds, radius, arc_bounds);
  }

  // Draw second ticker if enabled
  if (s_show_second_ticker && s_show_clock_analog) {
    outer_ring_draw_tickers(ctx, bounds, s_current_hour, s_current_minute, s_current_second);
  }
  if (s_show_clock_analog) {
    draw_gabbro_outer_ring_numbers(ctx, bounds);
  }
}

// ============================================================================
// TIME UPDATE FUNCTIONS
// ============================================================================

static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // Safety check
  if (!s_time_layer || !tick_time) {
    return;
  }
  
  // Only update step count, weather, battery on minute boundaries
  bool minute_changed = (tick_time->tm_min != s_last_weather_minute);
  if (minute_changed) {
    s_last_weather_minute = tick_time->tm_min;
    weather_display_module_update();
    if (s_show_step_tracker) {
      step_tracker_module_update();
    }
  }
  
  int step_count = s_show_step_tracker ? step_tracker_module_get_count() : 0;
  
  int distance_walked = 0;
#if defined(PBL_HEALTH)
  distance_walked = (int)health_service_sum_today(HealthMetricWalkedDistanceMeters);
#endif
  
  top_module_update(tick_time, s_top_module_format, step_count, distance_walked, s_use_miles);
  bottom_module_update(tick_time, s_bottom_module_format, step_count, distance_walked, s_use_miles);
  
  // Update time
  static char time_buffer[8];
  strftime(time_buffer, sizeof(time_buffer), check_if_24h() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, time_buffer);
  s_current_second = tick_time->tm_sec;
  s_current_minute = tick_time->tm_min;
  s_current_hour = tick_time->tm_hour;
  
  // Update AM/PM indicator only on hour change
  bool new_is_pm = (tick_time->tm_hour >= 12);
  if (new_is_pm != s_is_pm) {
    s_is_pm = new_is_pm;
    if (s_ampm_layer) {
      layer_mark_dirty(s_ampm_layer);
    }
  }
  
  // Only redraw canvas when second ticker is active or minute changed
  if (s_canvas_layer && (s_show_second_ticker || minute_changed)) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void accel_tap_handler(AccelAxisType axis, int32_t direction) {
  if (s_show_moon_view) {
    moon_view_module_show();
  }
}

// ============================================================================
// WINDOW HANDLERS
// ============================================================================

static void prv_window_load(Window *window) {
  
  window_set_background_color(window, GColorBlack);

  // Show splash screen if enabled
  if (s_style_logo > 0) {
    splash_logo_show(window, s_style_logo);
    
    // Schedule watchface UI load after splash
    app_timer_register(SPLASH_DURATION_MS, load_watchface_ui, window);
  } else {
    // Load watchface UI immediately
    load_watchface_ui(window);
  }
}

static void load_watchface_ui(void *data) {
  Window *window = (Window *)data;
  if (!window) return;
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Clean up splash logo (frees both layer and bitmap)
  splash_logo_cleanup();
  
  // Create time text layer (central element)
  // Position depends on 24h format (centered when 24h, offset when 12h for AM/PM)
  int time_width = check_if_24h() ? bounds.size.w : (bounds.size.w - 20);
  s_time_layer = text_layer_create(GRect(0, bounds.size.h / 2 - 20, time_width, 36));
  if (s_time_layer) {
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorWhite);
    text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS));
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  }
  
  // Create AM/PM indicator layer (only visible in 12h format)
  if (!check_if_24h()) {
    s_ampm_layer = layer_create(GRect(bounds.size.w / 2 + 36, bounds.size.h / 2 - 10, 18, 22));
    if (s_ampm_layer) {
      layer_set_update_proc(s_ampm_layer, ampm_update_proc);
      layer_add_child(window_layer, s_ampm_layer);
    }
  }
  
  // Create clock ring on its own layer (only redraws when settings change)
  s_clock_ring_layer = layer_create(bounds);
  if (s_clock_ring_layer) {
    layer_set_update_proc(s_clock_ring_layer, clock_ring_update_proc);
    layer_add_child(window_layer, s_clock_ring_layer);
  }
  
  // Create canvas layer for step tracker and second indicator
  // Add this LAST so the second ticker appears on top of all other elements
  s_canvas_layer = layer_create(bounds);
  if (s_canvas_layer) {
    layer_set_update_proc(s_canvas_layer, canvas_update_proc);
    layer_add_child(window_layer, s_canvas_layer);
  }
  
  // Initialize modules
  weather_module_set_scale(s_weather_scale);
  weather_display_module_init(window, bounds);
  weather_display_module_set_visible(s_show_weather);
  top_module_init(window, bounds);
  bottom_module_init(window, bounds);
  battery_module_init(window, bounds);
  
  if (s_show_step_tracker) {
    step_tracker_module_init(window, bounds, s_canvas_layer);
  }

  // Set step goal from settings
  if (s_show_step_tracker) {
    step_tracker_module_set_goal(s_step_goal);
  }

  // Initialize time display
  update_time();
}

static void prv_window_unload(Window *window) {
  // Destroy central text layers
  if (s_time_layer) {
    text_layer_destroy(s_time_layer);
    s_time_layer = NULL;
  }
  
  // Destroy custom layers
  if (s_canvas_layer) {
    layer_destroy(s_canvas_layer);
    s_canvas_layer = NULL;
  }
  if (s_clock_ring_layer) {
    layer_destroy(s_clock_ring_layer);
    s_clock_ring_layer = NULL;
  }
  if (s_ampm_layer) {
    layer_destroy(s_ampm_layer);
    s_ampm_layer = NULL;
  }
  
  // Deinitialize modules
  weather_display_module_deinit();
  top_module_deinit();
  bottom_module_deinit();
  battery_module_deinit();
  step_tracker_module_deinit();
}

// ============================================================================
// SERVICE HANDLERS
// ============================================================================

static void save_settings(void) {
  persist_write_bool(MESSAGE_KEY_SHOW_CLOCK_ANALOG, s_show_clock_analog);
  persist_write_bool(MESSAGE_KEY_SHOW_SECOND_TICKER, s_show_second_ticker);
  persist_write_bool(MESSAGE_KEY_SHOW_DECORATIVE_RING, s_show_decorative_ring);
  persist_write_int(MESSAGE_KEY_TOP_MODULE_FORMAT, s_top_module_format);
  persist_write_int(MESSAGE_KEY_BOTTOM_MODULE_FORMAT, s_bottom_module_format);
  persist_write_int(MESSAGE_KEY_STEP_GOAL, s_step_goal);
  persist_write_int(MESSAGE_KEY_SPLASH_LOGO_STYLE, s_style_logo);
  persist_write_bool(MESSAGE_KEY_SHOW_STEP_TRACKER, s_show_step_tracker);
  persist_write_bool(MESSAGE_KEY_SHOW_MOON_VIEW, s_show_moon_view);
  persist_write_bool(MESSAGE_KEY_SHOW_WEATHER, s_show_weather);
  persist_write_int(MESSAGE_KEY_WEATHER_SCALE, s_weather_scale);
  persist_write_bool(MESSAGE_KEY_USE_MILES, s_use_miles);
}

static void load_settings(void) {
  if (persist_exists(MESSAGE_KEY_SHOW_CLOCK_ANALOG)) {
    s_show_clock_analog = persist_read_bool(MESSAGE_KEY_SHOW_CLOCK_ANALOG);
  }
  if (persist_exists(MESSAGE_KEY_SHOW_SECOND_TICKER)) {
    s_show_second_ticker = persist_read_bool(MESSAGE_KEY_SHOW_SECOND_TICKER);
  }
  if (persist_exists(MESSAGE_KEY_SHOW_DECORATIVE_RING)) {
    s_show_decorative_ring = persist_read_bool(MESSAGE_KEY_SHOW_DECORATIVE_RING);
  }
  if (persist_exists(MESSAGE_KEY_TOP_MODULE_FORMAT)) {
    s_top_module_format = (DateFormatType)persist_read_int(MESSAGE_KEY_TOP_MODULE_FORMAT);
  }
  if (persist_exists(MESSAGE_KEY_BOTTOM_MODULE_FORMAT)) {
    s_bottom_module_format = (DateFormatType)persist_read_int(MESSAGE_KEY_BOTTOM_MODULE_FORMAT);
  }
  if (persist_exists(MESSAGE_KEY_STEP_GOAL)) {
    s_step_goal = persist_read_int(MESSAGE_KEY_STEP_GOAL);
    if (s_step_goal < 1000) s_step_goal = 8000;  // Validate stored value
  }
  if (persist_exists(MESSAGE_KEY_SPLASH_LOGO_STYLE)) {
    s_style_logo = persist_read_int(MESSAGE_KEY_SPLASH_LOGO_STYLE);
  }
  
  if (persist_exists(MESSAGE_KEY_SHOW_STEP_TRACKER)) {
    s_show_step_tracker = persist_read_bool(MESSAGE_KEY_SHOW_STEP_TRACKER);
  }
  if (persist_exists(MESSAGE_KEY_SHOW_MOON_VIEW)) {
    s_show_moon_view = persist_read_bool(MESSAGE_KEY_SHOW_MOON_VIEW);
  }
  if (persist_exists(MESSAGE_KEY_SHOW_WEATHER)) {
    s_show_weather = persist_read_bool(MESSAGE_KEY_SHOW_WEATHER);
  }
  if (persist_exists(MESSAGE_KEY_WEATHER_SCALE)) {
    s_weather_scale = persist_read_int(MESSAGE_KEY_WEATHER_SCALE);
  }
  if (persist_exists(MESSAGE_KEY_USE_MILES)) {
    s_use_miles = persist_read_bool(MESSAGE_KEY_USE_MILES);
  }
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  if (!iter) return;
  
  // Handle weather data
  Tuple *weather_tuple = dict_find(iter, MESSAGE_KEY_WEATHER_DATA);
  if (weather_tuple && weather_tuple->type == TUPLE_CSTRING) {
    weather_module_update(weather_tuple->value->cstring);
    weather_display_module_update();
    // Trigger UI update if needed
    if (s_canvas_layer) {
      layer_mark_dirty(s_canvas_layer);
    }
  }

  // Handle outer clock analog ring toggle
  Tuple *clock_analog_tuple = dict_find(iter, MESSAGE_KEY_SHOW_CLOCK_ANALOG);
  if (clock_analog_tuple) {
    s_show_clock_analog = (clock_analog_tuple->value->int32 == 1);
    if (s_canvas_layer) {
      layer_mark_dirty(s_canvas_layer);
    }
  }
  
  // Handle second ticker visibility setting
  Tuple *show_ticker_tuple = dict_find(iter, MESSAGE_KEY_SHOW_SECOND_TICKER);
  if (show_ticker_tuple) {
    s_show_second_ticker = (show_ticker_tuple->value->int32 == 1);
    // Switch tick frequency based on whether seconds are shown
    tick_timer_service_subscribe(s_show_second_ticker ? SECOND_UNIT : MINUTE_UNIT, tick_handler);
    if (s_canvas_layer) {
      layer_mark_dirty(s_canvas_layer);
    }
  }
  
  // Handle clock ring visibility setting
  Tuple *show_ring_tuple = dict_find(iter, MESSAGE_KEY_SHOW_DECORATIVE_RING);
  if (show_ring_tuple) {
    s_show_decorative_ring = (show_ring_tuple->value->int32 == 1);
    if (s_clock_ring_layer) {
      layer_mark_dirty(s_clock_ring_layer);
    }
  }
  
  // Handle step goal setting
  Tuple *step_goal_tuple = dict_find(iter, MESSAGE_KEY_STEP_GOAL);
  if (step_goal_tuple) {
    s_step_goal = atoi(step_goal_tuple->value->cstring);
    if (s_step_goal < 1000) s_step_goal = 1000;
    if (s_step_goal > 50000) s_step_goal = 50000;
    if (s_show_step_tracker) {
      step_tracker_module_set_goal(s_step_goal);
    }
    if (s_canvas_layer) {
      layer_mark_dirty(s_canvas_layer);
    }
  }
  
  // Handle splash logo style setting
  Tuple *logo_style_tuple = dict_find(iter, MESSAGE_KEY_SPLASH_LOGO_STYLE);
  if (logo_style_tuple) {
    s_style_logo = atoi(logo_style_tuple->value->cstring);
  }
  
  // Handle top module format setting
  Tuple *top_format_tuple = dict_find(iter, MESSAGE_KEY_TOP_MODULE_FORMAT);
  if (top_format_tuple) {
    s_top_module_format = parse_date_format(top_format_tuple->value->cstring);
  }
  
  // Handle bottom module format setting
  Tuple *bottom_format_tuple = dict_find(iter, MESSAGE_KEY_BOTTOM_MODULE_FORMAT);
  if (bottom_format_tuple) {
    s_bottom_module_format = parse_date_format(bottom_format_tuple->value->cstring);
  }
  
  // Handle show step tracker setting
  Tuple *show_tracker_tuple = dict_find(iter, MESSAGE_KEY_SHOW_STEP_TRACKER);
  if (show_tracker_tuple) {
    s_show_step_tracker = (show_tracker_tuple->value->int32 == 1);
    if (s_canvas_layer) {
      layer_mark_dirty(s_canvas_layer);
    }
    // Clock ring radius depends on step tracker visibility
    if (s_clock_ring_layer) {
      layer_mark_dirty(s_clock_ring_layer);
    }
  }
  
  // Handle show moon view setting
  Tuple *show_moon_tuple = dict_find(iter, MESSAGE_KEY_SHOW_MOON_VIEW);
  if (show_moon_tuple) {
    s_show_moon_view = (show_moon_tuple->value->int32 == 1);
  }
  
  // Handle show weather setting
  Tuple *show_weather_tuple = dict_find(iter, MESSAGE_KEY_SHOW_WEATHER);
  if (show_weather_tuple) {
    s_show_weather = (show_weather_tuple->value->int32 == 1);
    weather_display_module_set_visible(s_show_weather);
  }
  
  // Handle weather scale setting
  Tuple *weather_scale_tuple = dict_find(iter, MESSAGE_KEY_WEATHER_SCALE);
  if (weather_scale_tuple) {
    s_weather_scale = atoi(weather_scale_tuple->value->cstring);
    weather_module_set_scale(s_weather_scale);
    weather_display_module_update();
  }

  // Handle use miles setting
  Tuple *use_miles_tuple = dict_find(iter, MESSAGE_KEY_USE_MILES);
  if (use_miles_tuple) {
    s_use_miles = (use_miles_tuple->value->int32 == 1);
  }

  // Always persist settings regardless of UI state
  save_settings();

  // Only apply settings and redraw if the watchface UI has loaded
  if (s_canvas_layer) {
    
    // Handle step tracker enable/disable only when the setting actually changed
    if (show_tracker_tuple) {
      step_tracker_module_deinit();
      if (s_show_step_tracker) {
        step_tracker_module_init(s_window, layer_get_bounds(window_get_root_layer(s_window)), s_canvas_layer);
        step_tracker_module_set_goal(s_step_goal);
        step_tracker_module_subscribe();
      } else {
        step_tracker_module_unsubscribe();
      }
    } else if (step_goal_tuple && s_show_step_tracker) {
      step_tracker_module_set_goal(s_step_goal);
    }
    
    // Force full redraw of all elements
    layer_mark_dirty(s_canvas_layer);
    if (s_clock_ring_layer) {
      layer_mark_dirty(s_clock_ring_layer);
    }
    if (s_ampm_layer) {
      layer_mark_dirty(s_ampm_layer);
    }
    update_time();
    battery_module_update();
  }
}

// ============================================================================
// INITIALIZATION AND CLEANUP
// ============================================================================

static void prv_init(void) {
  // Load user settings
  load_settings();
  
  // Initialize weather module
  weather_module_init();
  
  // Load bitmap resources (only central)
  splash_logo_init();
  moon_view_module_init();

  if (!check_if_24h()) {
    s_am_active_bitmap = gbitmap_create_with_resource(RESOURCE_ID_AM_ACTIVE_IMAGE);
    s_am_inactive_bitmap = gbitmap_create_with_resource(RESOURCE_ID_AM_INACTIVE_IMAGE);
    s_pm_active_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PM_ACTIVE_IMAGE);
    s_pm_inactive_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PM_INACTIVE_IMAGE);
  }
  
  // Create and set up main window
  s_window = window_create();
  if (s_window) {
    window_set_window_handlers(s_window, (WindowHandlers) {
      .load = prv_window_load,
      .unload = prv_window_unload,
    });
    window_stack_push(s_window, true);
  }
  
  // Subscribe to services — use SECOND_UNIT only when second ticker is enabled
  tick_timer_service_subscribe(s_show_second_ticker && s_show_clock_analog ? SECOND_UNIT : MINUTE_UNIT, tick_handler);
  accel_tap_service_subscribe(accel_tap_handler);
  
  // Modules handle their own subscriptions
  battery_module_subscribe();
  if (s_show_step_tracker) {
    step_tracker_module_subscribe();
  }
  
  // Set up app message for config communication
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(512, 512);  // Increased buffer size for weather data
}

static void prv_deinit(void) {
  // Unsubscribe from services
  tick_timer_service_unsubscribe();
  accel_tap_service_unsubscribe();
  
  // Modules handle their own unsubscriptions
  battery_module_unsubscribe();
  step_tracker_module_unsubscribe();
  
  // Deinit moon view module
  moon_view_module_deinit();
  
  // Destroy window
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }
  
  // Destroy bitmap resources (only central/splash elements)
  splash_logo_cleanup();
  if (s_am_active_bitmap) {
    gbitmap_destroy(s_am_active_bitmap);
    s_am_active_bitmap = NULL;
  }
  if (s_am_inactive_bitmap) {
    gbitmap_destroy(s_am_inactive_bitmap);
    s_am_inactive_bitmap = NULL;
  }
  if (s_pm_active_bitmap) {
    gbitmap_destroy(s_pm_active_bitmap);
    s_pm_active_bitmap = NULL;
  }
  if (s_pm_inactive_bitmap) {
    gbitmap_destroy(s_pm_inactive_bitmap);
    s_pm_inactive_bitmap = NULL;
  }
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
  prv_init();
  app_event_loop();
  prv_deinit();
}
