#include "top_module.h"

static TextLayer *s_day_layer = NULL;
static BitmapLayer *s_walk_icon_layer = NULL;
static GBitmap *s_walk_icon_bitmap = NULL;
static DateFormatType s_current_format = DATE_FORMAT_WEEKDAY;

void top_module_init(Window *window, GRect bounds) {
  Layer *window_layer = window_get_root_layer(window);
  
  // Create text layer
  s_day_layer = text_layer_create(GRect(0, bounds.size.h / 2 - 40, bounds.size.w, 24));
  if (s_day_layer) {
    text_layer_set_background_color(s_day_layer, GColorClear);
    text_layer_set_text_color(s_day_layer, GColorWhite);
    text_layer_set_font(s_day_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(s_day_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_day_layer));
  }
  
  // Create walking icon layer (initially hidden)
  s_walk_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WALKING_IMAGE);
  s_walk_icon_layer = bitmap_layer_create(GRect(bounds.size.w / 2 + 20, bounds.size.h / 2 - 35, 15, 15));
  if (s_walk_icon_layer) {
    bitmap_layer_set_bitmap(s_walk_icon_layer, s_walk_icon_bitmap);
    bitmap_layer_set_compositing_mode(s_walk_icon_layer, GCompOpSet);
    layer_set_hidden(bitmap_layer_get_layer(s_walk_icon_layer), true);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_walk_icon_layer));
  }
}

void top_module_update(struct tm *tick_time, DateFormatType format, int step_count) {
  if (!s_day_layer) return;
  
  static char buffer[20];
  format_date_string(buffer, sizeof(buffer), tick_time, format, step_count);
  text_layer_set_text(s_day_layer, buffer);
  
  // Show walking icon only for step count format
  bool show_icon = (format == DATE_FORMAT_STEP_COUNT);
  if (s_walk_icon_layer) {
    layer_set_hidden(bitmap_layer_get_layer(s_walk_icon_layer), !show_icon);
  }
  
  s_current_format = format;
}

void top_module_deinit(void) {
  if (s_day_layer) {
    text_layer_destroy(s_day_layer);
    s_day_layer = NULL;
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
