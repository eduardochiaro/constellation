#include "weather_module.h"
#include <string.h>
#include <stdlib.h>

// ============================================================================
// PRIVATE STATE
// ============================================================================

static WeatherData s_weather_data = {0};
static int s_scale = 1; // 1=Celsius, 2=Fahrenheit

// ============================================================================
// PRIVATE FUNCTIONS - JSON PARSING HELPERS
// ============================================================================

// Simple JSON string value extractor (finds "key":"value" pattern)
static bool get_json_string(const char *json, const char *key, char *output, size_t max_len) {
  if (!json || !key || !output) return false;
  
  char search_pattern[64];
  snprintf(search_pattern, sizeof(search_pattern), "\"%s\":\"", key);
  
  const char *start = strstr(json, search_pattern);
  if (!start) return false;
  
  start += strlen(search_pattern);
  const char *end = strchr(start, '"');
  if (!end) return false;
  
  size_t len = end - start;
  if (len >= max_len) len = max_len - 1;
  
  strncpy(output, start, len);
  output[len] = '\0';
  
  return true;
}

// Simple JSON number value extractor (finds "key":number pattern)
static bool get_json_int(const char *json, const char *key, int32_t *output) {
  if (!json || !key || !output) return false;
  
  char search_pattern[64];
  snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", key);
  
  const char *start = strstr(json, search_pattern);
  if (!start) return false;
  
  start += strlen(search_pattern);
  
  // Skip whitespace
  while (*start == ' ' || *start == '\t' || *start == '\n') start++;
  
  // Parse number
  *output = atoi(start);
  
  return true;
}

// ============================================================================
// PUBLIC FUNCTIONS
// ============================================================================

void weather_module_init(void) {
  memset(&s_weather_data, 0, sizeof(WeatherData));
  s_weather_data.is_valid = false;
}

void weather_module_update(const char *json_data) {
  if (!json_data) {
    return;
  }
  
  int32_t temp_val;
  char str_val[32];
  
  // Parse temperature
  if (get_json_int(json_data, "temperature", &temp_val)) {
    s_weather_data.temperature = (int16_t)temp_val;
  }
  
  // Parse weather code
  if (get_json_int(json_data, "weatherCode", &temp_val)) {
    s_weather_data.weather_code = (uint16_t)temp_val;
  }
  
  // Parse sunrise
  if (get_json_string(json_data, "sunrise", str_val, sizeof(str_val))) {
    strncpy(s_weather_data.sunrise, str_val, sizeof(s_weather_data.sunrise) - 1);
  }
  
  // Parse sunset
  if (get_json_string(json_data, "sunset", str_val, sizeof(str_val))) {
    strncpy(s_weather_data.sunset, str_val, sizeof(s_weather_data.sunset) - 1);
  }
  
  // Parse moon phase
  if (get_json_int(json_data, "moonPhase", &temp_val)) {
    s_weather_data.moon_phase = (uint8_t)temp_val;
  }
  
  // Parse moon phase name
  if (get_json_string(json_data, "moonPhaseName", str_val, sizeof(str_val))) {
    strncpy(s_weather_data.moon_phase_name, str_val, sizeof(s_weather_data.moon_phase_name) - 1);
  }

  // Parse moon phase icon
  if (get_json_int(json_data, "moonPhaseIcon", &temp_val)) {
    s_weather_data.moon_phase_icon = (int16_t)temp_val;
  }
  
  s_weather_data.is_valid = true;
}

WeatherData* weather_module_get_data(void) {
  return &s_weather_data;
}

void weather_module_set_scale(int scale) {
  s_scale = (scale == 2) ? 2 : 1;
}

int16_t weather_module_get_temperature(void) {
  int16_t temp_c = s_weather_data.temperature;
  if (s_scale == 2) {
    // C to F: (C * 9 + 2) / 5 + 32  (integer math rounds toward nearest)
    return (int16_t)((temp_c * 9 + (temp_c >= 0 ? 2 : -2)) / 5 + 32);
  }
  return temp_c;
}

uint32_t weather_module_get_icon_resource(uint16_t code, bool is_night) {
  // if night and code is sun, use moon icon instead
  
  switch (code) {
    case 0:
    case 1:  return is_night ? RESOURCE_ID_WEATHER_MOON_IMAGE : RESOURCE_ID_WEATHER_SUN_IMAGE;
    case 2:  return is_night ? RESOURCE_ID_WEATHER_CLOUDY_MOON_IMAGE : RESOURCE_ID_WEATHER_CLOUDY_IMAGE;
    case 3:  return RESOURCE_ID_WEATHER_OVERCAST_IMAGE;
    case 45:
    case 48: return RESOURCE_ID_WEATHER_FOG_IMAGE;
    case 51:
    case 53:
    case 55:
    case 61:
    case 63:
    case 65:
    case 80:
    case 81:
    case 82: return RESOURCE_ID_WEATHER_RAINY_IMAGE;

    case 71:
    case 73:
    case 75:
    case 77:
    case 85:
    case 86: return RESOURCE_ID_WEATHER_SNOWY_IMAGE;

    case 95:
    case 96:
    case 99: return RESOURCE_ID_WEATHER_THUNDERSTORM_IMAGE;
    default: return RESOURCE_ID_WEATHER_EMPTY_IMAGE;
  }
}
