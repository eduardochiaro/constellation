#pragma once
#include <pebble.h>

// Shared date format types for top and bottom modules
typedef enum {
  DATE_FORMAT_WEEKDAY = 0,      // MONDAY
  DATE_FORMAT_MONTH_DAY,         // DECEMBER 29
  DATE_FORMAT_YYYY_MM_DD,        // 2025-12-29
  DATE_FORMAT_DD_MM_YYYY,        // 29/12/2025
  DATE_FORMAT_MM_DD_YYYY,        // 12/29/2025
  DATE_FORMAT_MONTH_YEAR,        // DEC 2025
  DATE_FORMAT_WEEKDAY_DAY,       // MON 29
  DATE_FORMAT_STEP_COUNT,        // Step count display
  DATE_FORMAT_DISTANCE,           // Distance walked display
  DATE_FORMAT_HEART_RATE          // Heart rate BPM display
} DateFormatType;

// Format a date string according to the specified format type
void format_date_string(char *buffer, size_t buffer_size, struct tm *tick_time, DateFormatType format, int step_count, int distance_walked, bool use_miles, int heart_rate);

bool from_string_to_tm(const char *time_str, struct tm *out);
