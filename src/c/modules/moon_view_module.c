#include "moon_view_module.h"

static Window *s_moon_window = NULL;
static TextLayer *s_moon_text_layer = NULL;

static void moon_view_timer_callback(void *data) {
  moon_view_module_hide();
}

static void moon_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  window_set_background_color(window, GColorBlack);
  
  s_moon_text_layer = text_layer_create(bounds);
  text_layer_set_background_color(s_moon_text_layer, GColorClear);
  text_layer_set_text_color(s_moon_text_layer, GColorWhite);
  text_layer_set_font(s_moon_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(s_moon_text_layer, GTextAlignmentCenter);
  text_layer_set_text(s_moon_text_layer, "Moon View");
  layer_add_child(window_layer, text_layer_get_layer(s_moon_text_layer));
  
  // Auto-dismiss after 5 seconds
  app_timer_register(MOON_VIEW_DURATION_MS, moon_view_timer_callback, NULL);
}

static void moon_window_unload(Window *window) {
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
