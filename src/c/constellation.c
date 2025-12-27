#include <pebble.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define BATTERY_WIDTH 25
#define BATTERY_HEIGHT 2
#define STEP_TRACK_WIDTH 15
#define STEP_TRACK_MARGIN 4
#define WALKING_ICON_SIZE 15
#define SECONDS_INDICATOR_SIZE 4
#define SPLASH_DURATION_MS 2000

// ============================================================================
// GLOBAL STATE - Windows and Layers
// ============================================================================

static Window *s_window;
static BitmapLayer *s_splash_logo_layer;

// Text layers
static TextLayer *s_day_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;

// Custom drawing layers
static Layer *s_canvas_layer;
static Layer *s_ampm_layer;
static Layer *s_battery_layer;

// Icon layers
static BitmapLayer *s_walk_layer;
static BitmapLayer *s_flag_layer;

// ============================================================================
// GLOBAL STATE - Bitmaps
// ============================================================================

static GBitmap *s_logo_bitmap;
static GBitmap *s_logo_color_bitmap;
static GBitmap *s_am_active_bitmap;
static GBitmap *s_am_inactive_bitmap;
static GBitmap *s_pm_active_bitmap;
static GBitmap *s_pm_inactive_bitmap;
static GBitmap *s_walking_bitmap;
static GBitmap *s_flag_bitmap;

// ============================================================================
// GLOBAL STATE - App Data
// ============================================================================

static bool s_is_pm;
static int s_step_count;
static int s_battery_percent = 100;
static int s_current_second;

// User settings (persisted)
static bool s_show_second_ticker = true;
static bool s_show_clock_ring = true;
static bool s_show_splash_screen = true;
static int s_step_goal = 8000;
static bool s_use_color_logo = false;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

static void load_watchface_ui(void *data);
#if defined(PBL_HEALTH)
static void health_handler(HealthEventType event, void *context);
#endif

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static void to_uppercase(char *str) {
  if (!str) return;
  
  while (*str) {
    if (*str >= 'a' && *str <= 'z') {
      *str = *str - 'a' + 'A';
    }
    str++;
  }
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

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  int filled_width = (s_battery_percent * bounds.size.w) / 100;
  
  // Clamp to valid range
  if (filled_width < 0) {
    filled_width = 0;
  } else if (filled_width > bounds.size.w) {
    filled_width = bounds.size.w;
  }

  // Draw background
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  // Draw filled portion
  if (filled_width > 0) {
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(bounds.origin.x, bounds.origin.y, filled_width, bounds.size.h), 
                       0, GCornerNone);
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

  // Draw clock ring with 60 tick marks (minute/second indicators) if enabled
  if (s_show_clock_ring) {
    GPoint ring_center = grect_center_point(&bounds);
    int ring_outer_radius = radius - STEP_TRACK_WIDTH - 3; // Just inside the step tracker
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
  
  // Draw base step track (dark gray arc from 90° to 270°)
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_radial(ctx, arc_bounds, GOvalScaleModeFitCircle, STEP_TRACK_WIDTH,
                       DEG_TO_TRIGANGLE(90), DEG_TO_TRIGANGLE(270));
  
  // Calculate and draw step progress
  float progress = (s_step_goal > 0) ? ((float)s_step_count / (float)s_step_goal) : 0;
  progress = (progress < 0) ? 0 : (progress > 1.0f) ? 1.0f : progress;

  if (progress > 0) {
    int32_t start_angle = 270 - (int32_t)(180.0f * progress);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_radial(ctx, arc_bounds, GOvalScaleModeFitCircle, STEP_TRACK_WIDTH,
                         DEG_TO_TRIGANGLE(start_angle), DEG_TO_TRIGANGLE(270));
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
  
  // Safety check: Don't update if UI hasn't been loaded yet
  if (!s_day_layer || !tick_time) {
    return;
  }
  
  // Update day of week
  static char day_buffer[16];
  strftime(day_buffer, sizeof(day_buffer), "%A", tick_time);
  to_uppercase(day_buffer);
  text_layer_set_text(s_day_layer, day_buffer);
  
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
  
  // Update step count
#if defined(PBL_HEALTH)
  s_step_count = (int)health_service_sum_today(HealthMetricStepCount);
#else
  s_step_count = 0;
#endif
  if (s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }

  // Update battery level
  BatteryChargeState charge = battery_state_service_peek();
  s_battery_percent = charge.charge_percent;
  if (s_battery_layer) {
    layer_mark_dirty(s_battery_layer);
  }
  
  // Update date
  static char date_buffer[20];
  strftime(date_buffer, sizeof(date_buffer), "%B %d", tick_time);
  to_uppercase(date_buffer);
  text_layer_set_text(s_date_layer, date_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

// ============================================================================
// WINDOW HANDLERS
// ============================================================================

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  window_set_background_color(window, GColorBlack);
  
  // Show splash screen if enabled
  if (s_show_splash_screen) {
    // Show splash logo (color or B&W based on user setting)
    GBitmap *splash_logo = s_use_color_logo ? s_logo_color_bitmap : s_logo_bitmap;
    if (splash_logo) {
      GRect logo_bounds = gbitmap_get_bounds(splash_logo);
      int x = (bounds.size.w - logo_bounds.size.w) / 2;
      int y = (bounds.size.h - logo_bounds.size.h) / 2;
      
      s_splash_logo_layer = bitmap_layer_create(GRect(x, y, logo_bounds.size.w, logo_bounds.size.h));
      if (s_splash_logo_layer) {
        bitmap_layer_set_bitmap(s_splash_logo_layer, splash_logo);
        layer_add_child(window_layer, bitmap_layer_get_layer(s_splash_logo_layer));
      }
    }
    
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
  
  // Clean up splash logo
  if (s_splash_logo_layer) {
    bitmap_layer_destroy(s_splash_logo_layer);
    s_splash_logo_layer = NULL;
  }
  
  // Create canvas layer for step tracker and second indicator
  s_canvas_layer = layer_create(bounds);
  if (s_canvas_layer) {
    layer_set_update_proc(s_canvas_layer, canvas_update_proc);
    layer_add_child(window_layer, s_canvas_layer);
  }
  
  // Create day of week text layer
  s_day_layer = text_layer_create(GRect(0, bounds.size.h / 2 - 40, bounds.size.w, 24));
  if (s_day_layer) {
    text_layer_set_background_color(s_day_layer, GColorClear);
    text_layer_set_text_color(s_day_layer, GColorWhite);
    text_layer_set_font(s_day_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(s_day_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_day_layer));
  }
  
  // Create time text layer
  s_time_layer = text_layer_create(GRect(0, bounds.size.h / 2 - 22, bounds.size.w - 20, 32));
  if (s_time_layer) {
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorWhite);
    text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_28_LIGHT_NUMBERS));
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  }
  
  // Create AM/PM indicator layer
  s_ampm_layer = layer_create(GRect(bounds.size.w / 2 + 30, bounds.size.h / 2 - 15, 18, 22));
  if (s_ampm_layer) {
    layer_set_update_proc(s_ampm_layer, ampm_update_proc);
    layer_add_child(window_layer, s_ampm_layer);
  }
  
  // Create date text layer
  s_date_layer = text_layer_create(GRect(0, bounds.size.h / 2 + 8, bounds.size.w, 24));
  if (s_date_layer) {
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, GColorWhite);
    text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  }

  // Create battery indicator layer
  s_battery_layer = layer_create(GRect((bounds.size.w - BATTERY_WIDTH) / 2,
                                       bounds.size.h / 2 + 8 + 24 + 5,
                                       BATTERY_WIDTH, BATTERY_HEIGHT));
  if (s_battery_layer) {
    layer_set_update_proc(s_battery_layer, battery_update_proc);
    layer_add_child(window_layer, s_battery_layer);
  }

  // Create walking icon layer
  if (s_walking_bitmap) {
    int icon_x = 5;
    int icon_y = bounds.size.h / 2 - WALKING_ICON_SIZE + 4;
#if defined(PBL_ROUND)
    icon_x += 8;
    icon_y -= 3;
#endif
    s_walk_layer = bitmap_layer_create(GRect(icon_x, icon_y, WALKING_ICON_SIZE, WALKING_ICON_SIZE));
    if (s_walk_layer) {
      bitmap_layer_set_bitmap(s_walk_layer, s_walking_bitmap);
      layer_add_child(window_layer, bitmap_layer_get_layer(s_walk_layer));
    }
  }

  // Create flag icon layer
  if (s_flag_bitmap) {
    int flag_x = bounds.size.w - WALKING_ICON_SIZE - 3;
    int flag_y = bounds.size.h / 2 - WALKING_ICON_SIZE + 4;
#if defined(PBL_ROUND)
    flag_x -= 8;
    flag_y -= 4;
#endif
    s_flag_layer = bitmap_layer_create(GRect(flag_x, flag_y, WALKING_ICON_SIZE, WALKING_ICON_SIZE));
    if (s_flag_layer) {
      bitmap_layer_set_bitmap(s_flag_layer, s_flag_bitmap);
      layer_add_child(window_layer, bitmap_layer_get_layer(s_flag_layer));
    }
  }

  // Initialize time display
  update_time();
}

static void prv_window_unload(Window *window) {
  // Destroy text layers
  if (s_day_layer) {
    text_layer_destroy(s_day_layer);
    s_day_layer = NULL;
  }
  if (s_time_layer) {
    text_layer_destroy(s_time_layer);
    s_time_layer = NULL;
  }
  if (s_date_layer) {
    text_layer_destroy(s_date_layer);
    s_date_layer = NULL;
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
  if (s_battery_layer) {
    layer_destroy(s_battery_layer);
    s_battery_layer = NULL;
  }
  
  // Destroy bitmap layers
  if (s_walk_layer) {
    bitmap_layer_destroy(s_walk_layer);
    s_walk_layer = NULL;
  }
  if (s_flag_layer) {
    bitmap_layer_destroy(s_flag_layer);
    s_flag_layer = NULL;
  }
  
  // Note: Bitmaps are destroyed in prv_deinit
}

// ============================================================================
// SERVICE HANDLERS
// ============================================================================

static void battery_handler(BatteryChargeState state) {
  s_battery_percent = state.charge_percent;
  if (s_battery_layer) {
    layer_mark_dirty(s_battery_layer);
  }
}

#if defined(PBL_HEALTH)
static void health_handler(HealthEventType event, void *context) {
  // Update step count when health data changes
  if (event == HealthEventMovementUpdate || event == HealthEventSignificantUpdate) {
    s_step_count = (int)health_service_sum_today(HealthMetricStepCount);
    if (s_canvas_layer) {
      layer_mark_dirty(s_canvas_layer);
    }
  }
}
#endif

static void save_settings(void) {
  persist_write_bool(MESSAGE_KEY_SHOW_SECOND_TICKER, s_show_second_ticker);
  persist_write_bool(MESSAGE_KEY_SHOW_CLOCK_RING, s_show_clock_ring);
  persist_write_bool(MESSAGE_KEY_SHOW_SPLASH_SCREEN, s_show_splash_screen);
  persist_write_int(MESSAGE_KEY_STEP_GOAL, s_step_goal);
  persist_write_bool(MESSAGE_KEY_SPLASH_LOGO_STYLE, s_use_color_logo);
}

static void load_settings(void) {
  if (persist_exists(MESSAGE_KEY_SHOW_SECOND_TICKER)) {
    s_show_second_ticker = persist_read_bool(MESSAGE_KEY_SHOW_SECOND_TICKER);
  }
  if (persist_exists(MESSAGE_KEY_SHOW_CLOCK_RING)) {
    s_show_clock_ring = persist_read_bool(MESSAGE_KEY_SHOW_CLOCK_RING);
  }
  if (persist_exists(MESSAGE_KEY_SHOW_SPLASH_SCREEN)) {
    s_show_splash_screen = persist_read_bool(MESSAGE_KEY_SHOW_SPLASH_SCREEN);
  }
  if (persist_exists(MESSAGE_KEY_STEP_GOAL)) {
    s_step_goal = persist_read_int(MESSAGE_KEY_STEP_GOAL);
    if (s_step_goal < 1000) s_step_goal = 8000;  // Validate stored value
  }
  if (persist_exists(MESSAGE_KEY_SPLASH_LOGO_STYLE)) {
    s_use_color_logo = persist_read_bool(MESSAGE_KEY_SPLASH_LOGO_STYLE);
  }
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  if (!iter) return;
  
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
  
  // Handle splash screen visibility setting
  Tuple *show_splash_tuple = dict_find(iter, MESSAGE_KEY_SHOW_SPLASH_SCREEN);
  if (show_splash_tuple) {
    s_show_splash_screen = (show_splash_tuple->value->int32 == 1);
  }
  
  // Handle step goal setting
  Tuple *step_goal_tuple = dict_find(iter, MESSAGE_KEY_STEP_GOAL);
  if (step_goal_tuple) {
    s_step_goal = atoi(step_goal_tuple->value->cstring);
    if (s_step_goal < 1000) s_step_goal = 1000;
    if (s_step_goal > 50000) s_step_goal = 50000;
    if (s_canvas_layer) {
      layer_mark_dirty(s_canvas_layer);
    }
  }
  
  // Handle splash logo style setting
  Tuple *logo_style_tuple = dict_find(iter, MESSAGE_KEY_SPLASH_LOGO_STYLE);
  if (logo_style_tuple && logo_style_tuple->value->cstring) {
    s_use_color_logo = (strcmp(logo_style_tuple->value->cstring, "color") == 0);
  }

  save_settings();
  layer_mark_dirty(s_canvas_layer);
}

// ============================================================================
// INITIALIZATION AND CLEANUP
// ============================================================================

static void prv_init(void) {
  // Load user settings
  load_settings();
  
  // Set up app message for config communication
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(256, 256);
  
  // Load bitmap resources
  s_logo_bitmap = gbitmap_create_with_resource(RESOURCE_ID_LOGO_BW_IMAGE);
  s_logo_color_bitmap = gbitmap_create_with_resource(RESOURCE_ID_LOGO_COLOR_IMAGE);
  s_am_active_bitmap = gbitmap_create_with_resource(RESOURCE_ID_AM_ACTIVE_IMAGE);
  s_am_inactive_bitmap = gbitmap_create_with_resource(RESOURCE_ID_AM_INACTIVE_IMAGE);
  s_pm_active_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PM_ACTIVE_IMAGE);
  s_pm_inactive_bitmap = gbitmap_create_with_resource(RESOURCE_ID_PM_INACTIVE_IMAGE);
  s_walking_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WALKING_IMAGE);
  s_flag_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FLAG_IMAGE);
  
  // Create and set up main window
  s_window = window_create();
  if (s_window) {
    window_set_window_handlers(s_window, (WindowHandlers) {
      .load = prv_window_load,
      .unload = prv_window_unload,
    });
    window_stack_push(s_window, true);
  }
  
  // Subscribe to services
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler);
  battery_state_service_subscribe(battery_handler);
  
#if defined(PBL_HEALTH)
  // Subscribe to health events for step tracking
  health_service_events_subscribe(health_handler, NULL);
  // Initialize step count
  s_step_count = (int)health_service_sum_today(HealthMetricStepCount);
#endif
}

static void prv_deinit(void) {
  // Unsubscribe from services
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  
#if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
#endif
  
  // Destroy window
  if (s_window) {
    window_destroy(s_window);
    s_window = NULL;
  }
  
  // Destroy bitmap resources
  if (s_logo_bitmap) {
    gbitmap_destroy(s_logo_bitmap);
    s_logo_bitmap = NULL;
  }
  if (s_logo_color_bitmap) {
    gbitmap_destroy(s_logo_color_bitmap);
    s_logo_color_bitmap = NULL;
  }
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
  if (s_walking_bitmap) {
    gbitmap_destroy(s_walking_bitmap);
    s_walking_bitmap = NULL;
  }
  if (s_flag_bitmap) {
    gbitmap_destroy(s_flag_bitmap);
    s_flag_bitmap = NULL;
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
