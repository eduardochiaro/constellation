#include "weather_display_module.h"
#include "weather_module.h"

static TextLayer *s_weather_layer = NULL;
static char s_weather_buffer[32];

void weather_display_module_init(Window *window, GRect bounds) {
  Layer *window_layer = window_get_root_layer(window);

  // Position above the top module (top module is at bounds.size.h / 2 - 40)
  s_weather_layer = text_layer_create(GRect(0, bounds.size.h / 2 - 65, bounds.size.w, 24));
  if (s_weather_layer) {
    text_layer_set_background_color(s_weather_layer, GColorClear);
    text_layer_set_text_color(s_weather_layer, GColorWhite);
    text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
  }
}

void weather_display_module_update(void) {
  if (!s_weather_layer) return;

  WeatherData *weather = weather_module_get_data();
  if (!weather->is_valid) {
    text_layer_set_text(s_weather_layer, "");
    return;
  }

  const char *desc = weather_module_get_description(weather->weather_code);
  snprintf(s_weather_buffer, sizeof(s_weather_buffer), "%d° %s", weather->temperature, desc);
  text_layer_set_text(s_weather_layer, s_weather_buffer);
}

void weather_display_module_deinit(void) {
  if (s_weather_layer) {
    text_layer_destroy(s_weather_layer);
    s_weather_layer = NULL;
  }
}
