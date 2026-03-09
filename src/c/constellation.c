#include <pebble.h>
#include "modules/top_module.h"
#include "modules/bottom_module.h"
#include "modules/battery_module.h"
#include "modules/step_tracker_module.h"
#include "modules/splash_logo_module.h"
#include "modules/moon_view_module.h"
#include "modules/weather_module.h"
#include "modules/weather_display_module.h"

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

// User settings (persisted)
static bool s_show_second_ticker = true;
static bool s_show_clock_ring = false;
static bool s_show_step_tracker = true;
static DateFormatType s_top_module_format = DATE_FORMAT_WEEKDAY;
static DateFormatType s_bottom_module_format = DATE_FORMAT_MONTH_DAY;
static int s_step_goal = 8000;
static int s_style_logo = 1;
static bool s_tracker_use_line = false;
static bool s_show_moon_view = true;
static bool s_show_weather = true;
static int s_weather_scale = 1;

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
  
  return DATE_FORMAT_WEEKDAY; // Default fallback
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


// Draws the static clock ring (tick marks) only when needed
static void draw_clock_ring(GContext *ctx, GRect bounds, int radius) {
  if (!s_show_clock_ring) return;
  GPoint ring_center = grect_center_point(&bounds);
  int ring_outer_radius = radius - STEP_TRACK_WIDTH - 3; // Just inside the step tracker
  if (!s_show_step_tracker) {
    ring_outer_radius += 10;
  }
  if (PBL_PLATFORM_TYPE_CURRENT == PlatformTypeEmery) {
    ring_outer_radius -= 15; // Adjust for round watches
  }
  int tick_length_normal = 2;  // Length of normal tick marks
  int tick_length_major = 5;   // Length of major tick marks (every 5th)
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  for (int i = 0; i < 60; i++) {
    int32_t angle = (TRIG_MAX_ANGLE * i) / 60;
    bool is_major = (i % 5 == 0);
    int tick_length = is_major ? tick_length_major : tick_length_normal;
    int ring_inner_radius = ring_outer_radius - tick_length;
    // Calculate outer point of tick mark
    GPoint outer = GPoint(
      ring_center.x + (int16_t)((sin_lookup(angle) * ring_outer_radius) / TRIG_MAX_RATIO),
      ring_center.y - (int16_t)((cos_lookup(angle) * ring_outer_radius) / TRIG_MAX_RATIO)
    );
    // Calculate inner point of tick mark
    GPoint inner = GPoint(
      ring_center.x + (int16_t)((sin_lookup(angle) * ring_inner_radius) / TRIG_MAX_RATIO),
      ring_center.y - (int16_t)((cos_lookup(angle) * ring_inner_radius) / TRIG_MAX_RATIO)
    );
    // Draw tick mark with appropriate thickness
    graphics_context_set_stroke_width(ctx, is_major ? 2 : 1);
    graphics_draw_line(ctx, outer, inner);
  }
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Calculate arc dimensions for step tracker based on platform
  int radius, diameter;
  GPoint center;
  GRect arc_bounds;

#if defined(PBL_ROUND)
  // Round watch layout
  const int BASE_RECT_WIDTH = 150;
  const int BASE_RECT_HEIGHT = 168;
  int x_offset = (bounds.size.w - BASE_RECT_WIDTH) / 2 + bounds.origin.x;
  int y_offset = (bounds.size.h - BASE_RECT_HEIGHT) / 2 + bounds.origin.y;

  radius = BASE_RECT_WIDTH / 2 + STEP_TRACK_MARGIN;
  diameter = radius * 2;
  center = GPoint(x_offset + BASE_RECT_WIDTH / 2, y_offset + BASE_RECT_HEIGHT + STEP_TRACK_MARGIN - 3);
  arc_bounds = GRect(center.x - radius, y_offset + 7, diameter, diameter);
#else
  // Rectangular watch layout
  radius = bounds.size.w / 2 + STEP_TRACK_MARGIN;
  diameter = radius * 2 - 15;
  center = GPoint(bounds.origin.x + bounds.size.w / 2, bounds.origin.y + bounds.size.h + STEP_TRACK_MARGIN - 3);
  arc_bounds = GRect(center.x - radius + 8, bounds.origin.y + 22, diameter, diameter);
#endif

  // Draw clock ring only if requested (not every second)
  draw_clock_ring(ctx, bounds, radius); // Only call this when settings change, not every second

  // Draw step tracker (delegated to module)
  if (s_show_step_tracker) {
    step_tracker_module_draw(layer, ctx, bounds, radius, arc_bounds, s_tracker_use_line);
  }

  // Calculate second indicator position
  GPoint indicator;

#if defined(PBL_ROUND)
  // Circular motion for round watches
  const int circle_inset = 6;
  GPoint screen_center = grect_center_point(&bounds);
  int circle_radius = bounds.size.w / 2 - circle_inset;
  if (circle_radius < 5) circle_radius = 5;
  int32_t angle = (TRIG_MAX_ANGLE * s_current_second) / 60;
  indicator = GPoint(
    screen_center.x + (int16_t)((sin_lookup(angle) * circle_radius) / TRIG_MAX_RATIO),
    screen_center.y - (int16_t)((cos_lookup(angle) * circle_radius) / TRIG_MAX_RATIO)
  );
#else
  // Perimeter motion for rectangular watches
  const int inset = 3;
  int left = bounds.origin.x + inset;
  int right = bounds.origin.x + bounds.size.w - 1 - inset;
  int top = bounds.origin.y + inset;
  int bottom = bounds.origin.y + bounds.size.h - 1 - inset;
  int width = right - left;
  int height = bottom - top;
  indicator = GPoint(left, top);
  if (width > 0 && height > 0) {
    int perimeter = 2 * (width + height);
    int dist = (perimeter * s_current_second) / 60;
    int offset = width / 2;
    dist = (dist + offset) % perimeter;
    // Determine position along perimeter
    if (dist <= width) {
      // Top edge
      indicator.x = left + dist;
      indicator.y = top;
    } else if (dist <= width + height) {
      // Right edge
      indicator.x = right;
      indicator.y = top + (dist - width);
    } else if (dist <= 2 * width + height) {
      // Bottom edge
      indicator.x = right - (dist - width - height);
      indicator.y = bottom;
    } else {
      // Left edge
      indicator.x = left;
      indicator.y = bottom - (dist - 2 * width - height);
    }
  }
#endif

  // Draw second ticker if enabled
  if (s_show_second_ticker) {
    int half = SECONDS_INDICATOR_SIZE / 2;
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorRed, GColorWhite));
    graphics_fill_rect(ctx, GRect(indicator.x - half, indicator.y - half,
                                  SECONDS_INDICATOR_SIZE, SECONDS_INDICATOR_SIZE), 
                       0, GCornerNone);
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
  
  // Get current step count (only if step tracker is enabled)
  int step_count = s_show_step_tracker ? step_tracker_module_get_count() : 0;
  
  // Update modules
  weather_display_module_update();
  top_module_update(tick_time, s_top_module_format, step_count);
  bottom_module_update(tick_time, s_bottom_module_format, step_count);
  
  // Update time
  static char time_buffer[8];
  strftime(time_buffer, sizeof(time_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);
  text_layer_set_text(s_time_layer, time_buffer);
  s_current_second = tick_time->tm_sec;
  
  // Update AM/PM indicator
  s_is_pm = (tick_time->tm_hour >= 12);
  if (s_ampm_layer) {
    layer_mark_dirty(s_ampm_layer);
  }
  
  // Update step tracker and battery
  if (s_show_step_tracker) {
    step_tracker_module_update();
  }
  battery_module_update();
  
  if (s_canvas_layer) {
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
  int time_width = clock_is_24h_style() ? bounds.size.w : (bounds.size.w - 20);
  s_time_layer = text_layer_create(GRect(0, bounds.size.h / 2 - 22, time_width, 32));
  if (s_time_layer) {
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorWhite);
    text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_28_LIGHT_NUMBERS));
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  }
  
  // Create AM/PM indicator layer (only visible in 12h format)
  if (!clock_is_24h_style()) {
    s_ampm_layer = layer_create(GRect(bounds.size.w / 2 + 30, bounds.size.h / 2 - 15, 18, 22));
    if (s_ampm_layer) {
      layer_set_update_proc(s_ampm_layer, ampm_update_proc);
      layer_add_child(window_layer, s_ampm_layer);
    }
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
  persist_write_bool(MESSAGE_KEY_SHOW_SECOND_TICKER, s_show_second_ticker);
  persist_write_bool(MESSAGE_KEY_SHOW_CLOCK_RING, s_show_clock_ring);
  persist_write_int(MESSAGE_KEY_TOP_MODULE_FORMAT, s_top_module_format);
  persist_write_int(MESSAGE_KEY_BOTTOM_MODULE_FORMAT, s_bottom_module_format);
  persist_write_int(MESSAGE_KEY_STEP_GOAL, s_step_goal);
  persist_write_int(MESSAGE_KEY_SPLASH_LOGO_STYLE, s_style_logo);
  persist_write_bool(MESSAGE_KEY_TRACKER_STYLE, s_tracker_use_line);
  persist_write_bool(MESSAGE_KEY_SHOW_STEP_TRACKER, s_show_step_tracker);
  persist_write_bool(MESSAGE_KEY_SHOW_MOON_VIEW, s_show_moon_view);
  persist_write_bool(MESSAGE_KEY_SHOW_WEATHER, s_show_weather);
  persist_write_int(MESSAGE_KEY_WEATHER_SCALE, s_weather_scale);
}

static void load_settings(void) {
  if (persist_exists(MESSAGE_KEY_SHOW_SECOND_TICKER)) {
    s_show_second_ticker = persist_read_bool(MESSAGE_KEY_SHOW_SECOND_TICKER);
  }
  if (persist_exists(MESSAGE_KEY_SHOW_CLOCK_RING)) {
    s_show_clock_ring = persist_read_bool(MESSAGE_KEY_SHOW_CLOCK_RING);
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
  if (persist_exists(MESSAGE_KEY_TRACKER_STYLE)) {
    s_tracker_use_line = persist_read_bool(MESSAGE_KEY_TRACKER_STYLE);
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
  
  // Handle second ticker visibility setting
  Tuple *show_ticker_tuple = dict_find(iter, MESSAGE_KEY_SHOW_SECOND_TICKER);
  if (show_ticker_tuple) {
    s_show_second_ticker = (show_ticker_tuple->value->int32 == 1);
    if (s_canvas_layer) {
      layer_mark_dirty(s_canvas_layer);
    }
  }
  
  // Handle clock ring visibility setting
  Tuple *show_ring_tuple = dict_find(iter, MESSAGE_KEY_SHOW_CLOCK_RING);
  if (show_ring_tuple) {
    s_show_clock_ring = (show_ring_tuple->value->int32 == 1);
    if (s_canvas_layer) {
      layer_mark_dirty(s_canvas_layer);
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
  
  // Handle step tracker style setting
  Tuple *tracker_style_tuple = dict_find(iter, MESSAGE_KEY_TRACKER_STYLE);
  if (tracker_style_tuple) {
    s_tracker_use_line = (tracker_style_tuple->value->int32 == 1);
    moon_view_module_set_line_style(s_tracker_use_line);
    if (s_canvas_layer) {
      layer_mark_dirty(s_canvas_layer);
    }
  }
  
  // Handle show step tracker setting
  Tuple *show_tracker_tuple = dict_find(iter, MESSAGE_KEY_SHOW_STEP_TRACKER);
  if (show_tracker_tuple) {
    s_show_step_tracker = (show_tracker_tuple->value->int32 == 1);
    if (s_canvas_layer) {
      layer_mark_dirty(s_canvas_layer);
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

  // Only apply settings and redraw if the watchface UI has loaded
  if (s_canvas_layer) {
    save_settings();
    
    // Handle step tracker enable/disable (always deinit first to prevent double-init leak)
    step_tracker_module_deinit();
    if (s_show_step_tracker) {
      step_tracker_module_init(s_window, layer_get_bounds(window_get_root_layer(s_window)), s_canvas_layer);
      step_tracker_module_set_goal(s_step_goal);
    }
    
    // Force full redraw of all elements
    layer_mark_dirty(s_canvas_layer);
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
  moon_view_module_set_line_style(s_tracker_use_line);
  if (!clock_is_24h_style()) {
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
  
  // Subscribe to services (central only)
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
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
