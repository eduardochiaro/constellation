#include "battery_module.h"

static Layer *s_battery_layer = NULL;
static int s_battery_percent = 100;

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

static void battery_handler(BatteryChargeState state) {
  s_battery_percent = state.charge_percent;
  if (s_battery_layer) {
    layer_mark_dirty(s_battery_layer);
  }
}

void battery_module_init(Window *window, GRect bounds) {
  Layer *window_layer = window_get_root_layer(window);
  
  // Create battery indicator layer
  s_battery_layer = layer_create(GRect((bounds.size.w - BATTERY_WIDTH) / 2,
                                       bounds.size.h / 2 + 8 + 24 + 5,
                                       BATTERY_WIDTH, BATTERY_HEIGHT));
  if (s_battery_layer) {
    layer_set_update_proc(s_battery_layer, battery_update_proc);
    layer_add_child(window_layer, s_battery_layer);
  }
  
  // Initialize battery level
  BatteryChargeState charge = battery_state_service_peek();
  s_battery_percent = charge.charge_percent;
}

void battery_module_update(void) {
  BatteryChargeState charge = battery_state_service_peek();
  s_battery_percent = charge.charge_percent;
  if (s_battery_layer) {
    layer_mark_dirty(s_battery_layer);
  }
}

void battery_module_subscribe(void) {
  battery_state_service_subscribe(battery_handler);
}

void battery_module_unsubscribe(void) {
  battery_state_service_unsubscribe();
}

void battery_module_deinit(void) {
  if (s_battery_layer) {
    layer_destroy(s_battery_layer);
    s_battery_layer = NULL;
  }
}
