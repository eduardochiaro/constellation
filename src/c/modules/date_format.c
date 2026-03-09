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

void format_date_string(char *buffer, size_t buffer_size, struct tm *tick_time, DateFormatType format, int step_count) {
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
      
    default:
      // Fallback to weekday
      strftime(buffer, buffer_size, "%A", tick_time);
      to_uppercase(buffer);
      break;
  }
}

struct tm *from_string_to_tm(const char *time_str) {
  // Parse "2026-03-08T06:30" manually (no sscanf to avoid pulling in libc)
  static struct tm t;
  memset(&t, 0, sizeof(struct tm));
  if (!time_str || strlen(time_str) < 16) return NULL;

  t.tm_year = atoi(time_str) - 1900;       // "2026" -> 126
  t.tm_mon  = atoi(time_str + 5) - 1;      // "03"   -> 2
  t.tm_mday = atoi(time_str + 8);           // "08"
  t.tm_hour = atoi(time_str + 11);          // "06"
  t.tm_min  = atoi(time_str + 14);          // "30"
  return &t;
}
