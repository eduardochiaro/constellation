#include "bottom_module.h"

static TextLayer *s_date_layer = NULL;
static BitmapLayer *s_walk_icon_layer = NULL;
static GBitmap *s_walk_icon_bitmap = NULL;
static DateFormatType s_current_format = DATE_FORMAT_MONTH_DAY;

void bottom_module_init(Window *window, GRect bounds, int text_y_offset, uint32_t walk_icon_res, int icon_y_offset) {
  Layer *window_layer = window_get_root_layer(window);
  
  // Create text layer
  s_date_layer = text_layer_create(GRect(0, bounds.size.h / 2 + text_y_offset, bounds.size.w, 24));
  if (s_date_layer) {
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, GColorWhite);
    text_layer_set_font(s_date_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  }
  
  // Create walking icon layer (initially hidden)
  s_walk_icon_bitmap = gbitmap_create_with_resource(walk_icon_res);
  s_walk_icon_layer = bitmap_layer_create(GRect(bounds.size.w / 2 + 20, bounds.size.h / 2 + icon_y_offset, 15, 15));
  if (s_walk_icon_layer) {
    bitmap_layer_set_bitmap(s_walk_icon_layer, s_walk_icon_bitmap);
    bitmap_layer_set_compositing_mode(s_walk_icon_layer, GCompOpSet);
    layer_set_hidden(bitmap_layer_get_layer(s_walk_icon_layer), true);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_walk_icon_layer));
  }
}

void bottom_module_update(struct tm *tick_time, DateFormatType format, int step_count, int distance_walked, bool use_miles, int heart_rate) {
  if (!s_date_layer) return;
  
  static char buffer[20];
  format_date_string(buffer, sizeof(buffer), tick_time, format, step_count, distance_walked, use_miles, heart_rate);
  text_layer_set_text(s_date_layer, buffer);
  
  // Show walking icon for step count or distance format
  bool show_icon = (format == DATE_FORMAT_STEP_COUNT);
  if (s_walk_icon_layer) {
    layer_set_hidden(bitmap_layer_get_layer(s_walk_icon_layer), !show_icon);
  }
  
  s_current_format = format;
}

void bottom_module_deinit(void) {
  if (s_date_layer) {
    text_layer_destroy(s_date_layer);
    s_date_layer = NULL;
  }
  
  if (s_walk_icon_layer) {
    bitmap_layer_destroy(s_walk_icon_layer);
    s_walk_icon_layer = NULL;
  }
  
  if (s_walk_icon_bitmap) {
    gbitmap_destroy(s_walk_icon_bitmap);
    s_walk_icon_bitmap = NULL;
  }
}
