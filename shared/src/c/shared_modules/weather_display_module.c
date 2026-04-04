#include "weather_display_module.h"
#include "../utilities/weather.h"
#include "../utilities/date_format.h"

#define WEATHER_ICON_SIZE 15

static TextLayer *s_weather_layer = NULL;
static BitmapLayer *s_icon_layer = NULL;
static GBitmap *s_icon_bitmap = NULL;
static char s_weather_buffer[12];
static int s_center_y;
static int s_center_x;
static bool s_visible = true;
static uint32_t s_last_res_id = 0;
static int16_t s_last_temp = INT16_MIN;
static int s_cached_day = -1;
static int s_sunrise_min = -1;
static int s_sunset_min = -1;

void weather_display_module_init(Window *window, GRect bounds, int weather_y_offset) {
  Layer *window_layer = window_get_root_layer(window);

  s_center_x = bounds.size.w / 2;
  s_center_y = bounds.size.h / 2 + weather_y_offset;

  // Temperature text — right of center, leaves room for icon on the left
  s_weather_layer = text_layer_create(GRect(0, s_center_y, s_center_x, 24));
  if (s_weather_layer) {
    text_layer_set_background_color(s_weather_layer, GColorClear);
    text_layer_set_text_color(s_weather_layer, GColorWhite);
    text_layer_set_font(s_weather_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(s_weather_layer, GTextAlignmentRight);
    layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
  }

  // Weather icon — left of center
  s_icon_layer = bitmap_layer_create(GRect(s_center_x + (WEATHER_ICON_SIZE /2) , s_center_y + 6, WEATHER_ICON_SIZE, WEATHER_ICON_SIZE));
  if (s_icon_layer) {
    bitmap_layer_set_compositing_mode(s_icon_layer, GCompOpSet);
    bitmap_layer_set_background_color(s_icon_layer, GColorClear);
    layer_set_hidden(bitmap_layer_get_layer(s_icon_layer), true);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_icon_layer));
  }
}

void weather_display_module_update(void) {
  if (!s_weather_layer) return;

  if (!s_visible) return;

  WeatherData *weather = weather_module_get_data();
  if (!weather || !weather->is_valid) {
    text_layer_set_text(s_weather_layer, "");
    if (s_icon_layer) layer_set_hidden(bitmap_layer_get_layer(s_icon_layer), true);
    s_last_res_id = 0;
    s_last_temp = INT16_MIN;
    return;
  }

  // Update temperature text
  int16_t temp = weather_module_get_temperature();
  if (temp != s_last_temp) {
    s_last_temp = temp;
    snprintf(s_weather_buffer, sizeof(s_weather_buffer), "%d°", temp);
    text_layer_set_text(s_weather_layer, s_weather_buffer);
  }

  bool is_night = false;
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  if (t) {
    // Recalculate sunrise/sunset only when the day changes
    if (t->tm_mday != s_cached_day) {
      s_cached_day = t->tm_mday;
      struct tm t_sunrise, t_sunset;
      if (from_string_to_tm(weather->sunrise, &t_sunrise) &&
          from_string_to_tm(weather->sunset, &t_sunset)) {
        s_sunrise_min = t_sunrise.tm_hour * 60 + t_sunrise.tm_min;
        s_sunset_min = t_sunset.tm_hour * 60 + t_sunset.tm_min;
      }
    }
    if (s_sunrise_min >= 0 && s_sunset_min >= 0) {
      int now_min = t->tm_hour * 60 + t->tm_min;
      is_night = (now_min < s_sunrise_min) || (now_min >= s_sunset_min);
    }
  }

  // Update icon only if resource changed
  uint32_t res_id = weather_module_get_icon_resource(weather->weather_code, is_night);
  if (res_id != s_last_res_id) {
    s_last_res_id = res_id;
    if (s_icon_bitmap) {
      gbitmap_destroy(s_icon_bitmap);
      s_icon_bitmap = NULL;
    }
    s_icon_bitmap = gbitmap_create_with_resource(res_id);
    if (s_icon_bitmap && s_icon_layer) {
      bitmap_layer_set_bitmap(s_icon_layer, s_icon_bitmap);
      layer_set_hidden(bitmap_layer_get_layer(s_icon_layer), false);
    }
  }
}

void weather_display_module_set_visible(bool visible) {
  s_visible = visible;
  if (s_weather_layer) {
    layer_set_hidden(text_layer_get_layer(s_weather_layer), !visible);
  }
  if (s_icon_layer) {
    layer_set_hidden(bitmap_layer_get_layer(s_icon_layer), !visible);
  }
}

void weather_display_module_deinit(void) {
  if (s_weather_layer) {
    text_layer_destroy(s_weather_layer);
    s_weather_layer = NULL;
  }
  if (s_icon_layer) {
    bitmap_layer_destroy(s_icon_layer);
    s_icon_layer = NULL;
  }
  if (s_icon_bitmap) {
    gbitmap_destroy(s_icon_bitmap);
    s_icon_bitmap = NULL;
  }
}
