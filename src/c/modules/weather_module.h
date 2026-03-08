#pragma once
#include <pebble.h>

// Weather data structure
typedef struct {
  int16_t temperature;      // Current temperature in °C
  uint8_t humidity;         // Relative humidity %
  uint16_t weather_code;    // WMO Weather code
  uint8_t wind_speed;       // Wind speed in km/h
  int16_t temp_max;         // Today's max temperature
  int16_t temp_min;         // Today's min temperature
  char sunrise[20];         // Sunrise time ISO format
  char sunset[20];          // Sunset time ISO format
  uint8_t moon_phase;       // Moon phase 0-100 (0=new, 50=full, 100=new)
  char moon_phase_name[20]; // Moon phase name
  int16_t moon_phase_icon; // Moon phase icon
  time_t timestamp;         // When data was fetched
  bool is_valid;            // Whether we have valid data
} WeatherData;

// Initialize weather module
void weather_module_init(void);

// Deinitialize weather module
void weather_module_deinit(void);

// Update weather data from JSON string
void weather_module_update(const char *json_data);

// Get current weather data
WeatherData* weather_module_get_data(void);

// Get weather description from WMO code
const char* weather_module_get_description(uint16_t code);

// Get weather icon character from WMO code (for display)
const char* weather_module_get_icon(uint16_t code);
