#include "weather_module.h"
#include <string.h>
#include <stdlib.h>

// ============================================================================
// PRIVATE STATE
// ============================================================================

static WeatherData s_weather_data = {0};

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

void weather_module_deinit(void) {
  // Nothing to clean up
}

void weather_module_update(const char *json_data) {
  if (!json_data) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Weather update: NULL data");
    return;
  }
  
  APP_LOG(APP_LOG_LEVEL_INFO, "Parsing weather data: %s", json_data);
  
  int32_t temp_val;
  char str_val[32];
  
  // Parse temperature
  if (get_json_int(json_data, "temperature", &temp_val)) {
    s_weather_data.temperature = (int16_t)temp_val;
  }
  
  // Parse humidity
  if (get_json_int(json_data, "humidity", &temp_val)) {
    s_weather_data.humidity = (uint8_t)temp_val;
  }
  
  // Parse weather code
  if (get_json_int(json_data, "weatherCode", &temp_val)) {
    s_weather_data.weather_code = (uint16_t)temp_val;
  }
  
  // Parse wind speed
  if (get_json_int(json_data, "windSpeed", &temp_val)) {
    s_weather_data.wind_speed = (uint8_t)temp_val;
  }
  
  // Parse max temperature
  if (get_json_int(json_data, "tempMax", &temp_val)) {
    s_weather_data.temp_max = (int16_t)temp_val;
  }
  
  // Parse min temperature
  if (get_json_int(json_data, "tempMin", &temp_val)) {
    s_weather_data.temp_min = (int16_t)temp_val;
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
  
  // Parse timestamp
  if (get_json_int(json_data, "timestamp", &temp_val)) {
    s_weather_data.timestamp = (time_t)(temp_val / 1000); // Convert from ms to seconds
  }
  
  s_weather_data.is_valid = true;
  
  APP_LOG(APP_LOG_LEVEL_INFO, "Weather updated: %d°C, code: %d, moon: %d%%", 
          s_weather_data.temperature, s_weather_data.weather_code, s_weather_data.moon_phase);
}

WeatherData* weather_module_get_data(void) {
  return &s_weather_data;
}

// WMO Weather interpretation codes
// https://open-meteo.com/en/docs
const char* weather_module_get_description(uint16_t code) {
  switch (code) {
    case 0: return "Clear sky";
    case 1: return "Mainly clear";
    case 2: return "Partly cloudy";
    case 3: return "Overcast";
    case 45: return "Fog";
    case 48: return "Rime fog";
    case 51: return "Light drizzle";
    case 53: return "Drizzle";
    case 55: return "Heavy drizzle";
    case 61: return "Light rain";
    case 63: return "Rain";
    case 65: return "Heavy rain";
    case 71: return "Light snow";
    case 73: return "Snow";
    case 75: return "Heavy snow";
    case 77: return "Snow grains";
    case 80: return "Light showers";
    case 81: return "Showers";
    case 82: return "Heavy showers";
    case 85: return "Light snow showers";
    case 86: return "Snow showers";
    case 95: return "Thunderstorm";
    case 96: return "Thunderstorm + hail";
    case 99: return "Heavy thunderstorm";
    default: return "Unknown";
  }
}

const char* weather_module_get_icon(uint16_t code) {
  // Return simple text representations
  switch (code) {
    case 0:
    case 1: return "☀";
    case 2: return "⛅";
    case 3: return "☁";
    case 45:
    case 48: return "🌫";
    case 51:
    case 53:
    case 55:
    case 61:
    case 63:
    case 65:
    case 80:
    case 81:
    case 82: return "🌧";
    case 71:
    case 73:
    case 75:
    case 77:
    case 85:
    case 86: return "❄";
    case 95:
    case 96:
    case 99: return "⚡";
    default: return "?";
  }
}
