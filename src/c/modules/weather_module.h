#pragma once
#include <pebble.h>

// Weather data structure
typedef struct {
  int16_t temperature;      // Current temperature in °C
  uint16_t weather_code;    // WMO Weather code
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

// Update weather data from JSON string
void weather_module_update(const char *json_data);

// Get current weather data
WeatherData* weather_module_get_data(void);

// Get weather icon resource ID from WMO code
uint32_t weather_module_get_icon_resource(uint16_t code);

// Set temperature scale (1=Celsius, 2=Fahrenheit)
void weather_module_set_scale(int scale);

// Get temperature in the configured scale (rounded, no decimals)
int16_t weather_module_get_temperature(void);
