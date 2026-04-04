#include "date_format.h"
#include <string.h>
#include <stdio.h>

static void to_uppercase(char *str) {
  if (!str) return;
  
  while (*str) {
    if (*str >= 'a' && *str <= 'z') {
      *str = *str - 'a' + 'A';
    }
    str++;
  }
}

void format_date_string(char *buffer, size_t buffer_size, struct tm *tick_time, DateFormatType format, int step_count, int distance_walked, bool use_miles, int heart_rate) {
  if (!buffer) return;
  
  switch (format) {
    case DATE_FORMAT_WEEKDAY:
      // MONDAY
      if (!tick_time) return;
      strftime(buffer, buffer_size, "%A", tick_time);
      to_uppercase(buffer);
      break;
      
    case DATE_FORMAT_MONTH_DAY:
      // DECEMBER 29
      if (!tick_time) return;
      strftime(buffer, buffer_size, "%B %d", tick_time);
      to_uppercase(buffer);
      break;
      
    case DATE_FORMAT_YYYY_MM_DD:
      // 2025-12-29
      if (!tick_time) return;
      strftime(buffer, buffer_size, "%Y-%m-%d", tick_time);
      break;
      
    case DATE_FORMAT_DD_MM_YYYY:
      // 29/12/2025
      if (!tick_time) return;
      strftime(buffer, buffer_size, "%d/%m/%Y", tick_time);
      break;
      
    case DATE_FORMAT_MM_DD_YYYY:
      // 12/29/2025
      if (!tick_time) return;
      strftime(buffer, buffer_size, "%m/%d/%Y", tick_time);
      break;
      
    case DATE_FORMAT_MONTH_YEAR:
      // DEC 2025
      if (!tick_time) return;
      strftime(buffer, buffer_size, "%b %Y", tick_time);
      to_uppercase(buffer);
      break;
      
    case DATE_FORMAT_WEEKDAY_DAY:
      // MON 29
      if (!tick_time) return;
      strftime(buffer, buffer_size, "%a %d", tick_time);
      to_uppercase(buffer);
      break;
      
    case DATE_FORMAT_STEP_COUNT:
      // Step count
      snprintf(buffer, buffer_size, "%d", step_count);
      break;
      
    case DATE_FORMAT_DISTANCE:
      // Distance walked
      if (use_miles) {
        // Convert meters to miles (1 mile = 1609.34m)
        int feet = (distance_walked * 3281) / 1000; // meters to feet*10 / 10
        if (feet >= 52800) { // >= 1 mile in feet (5280 * 10)
          int miles_x10 = (distance_walked * 10) / 1609;
          snprintf(buffer, buffer_size, "%d.%d MI", miles_x10 / 10, miles_x10 % 10);
        } else {
          int ft = (distance_walked * 3281) / 1000;
          snprintf(buffer, buffer_size, "%d FT", ft);
        }
      } else {
        if (distance_walked >= 1000) {
          int km_x10 = distance_walked / 100; // e.g. 1500m → 15 → "1.5 KM"
          snprintf(buffer, buffer_size, "%d.%d KM", km_x10 / 10, km_x10 % 10);
        } else {
          snprintf(buffer, buffer_size, "%d M", distance_walked);
        }
      }
      break;
      
    case DATE_FORMAT_HEART_RATE:
      // Heart rate BPM
      if (heart_rate > 0) {
        snprintf(buffer, buffer_size, "%d BPM", heart_rate);
      } else {
        snprintf(buffer, buffer_size, "-- BPM");
      }
      break;
      
    default:
      // Fallback to weekday
      strftime(buffer, buffer_size, "%A", tick_time);
      to_uppercase(buffer);
      break;
  }
}

bool from_string_to_tm(const char *time_str, struct tm *out) {
  // Parse "2026-03-08T06:30" manually (no sscanf to avoid pulling in libc)
  if (!time_str || !out || strlen(time_str) < 16) return false;

  memset(out, 0, sizeof(struct tm));
  out->tm_year = atoi(time_str) - 1900;       // "2026" -> 126
  out->tm_mon  = atoi(time_str + 5) - 1;      // "03"   -> 2
  out->tm_mday = atoi(time_str + 8);           // "08"
  out->tm_hour = atoi(time_str + 11);          // "06"
  out->tm_min  = atoi(time_str + 14);          // "30"
  return true;
}
