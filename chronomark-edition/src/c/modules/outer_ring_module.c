#include "outer_ring_module.h"

#define TICKER_RADIAL_DEPTH 25
#define TICKER_TIP_SIZE 6

void outer_ring_draw(GContext *ctx, GRect bounds) {
  GPoint screen_center = grect_center_point(&bounds);
  int outer_radius = bounds.size.w / 2 - 2;
  int inner_radius = outer_radius - 26;

  graphics_context_set_stroke_color(ctx, GColorLightGray);
  graphics_context_set_stroke_width(ctx, 2);
  graphics_draw_circle(ctx, screen_center, outer_radius);

  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 1);
  graphics_draw_circle(ctx, screen_center, inner_radius);
}

void outer_ring_draw_numbers(GContext *ctx, GRect bounds) {
  GPoint screen_center = grect_center_point(&bounds);
  int outer_radius = bounds.size.w / 2 - 2;
  int inner_radius = outer_radius - 26;

  int num_radius = (outer_radius + inner_radius) / 2;
  GFont font = fonts_get_system_font(FONT_KEY_LECO_20_BOLD_NUMBERS);
  const char *numbers[] = { "12", "3", "6", "9" };
  const int angles[] = { 0, 90, 180, 270 };

  for (int i = 0; i < 4; i++) {
    int32_t trig_angle = DEG_TO_TRIGANGLE(angles[i]);
    int nx = screen_center.x + (int16_t)((sin_lookup(trig_angle) * num_radius) / TRIG_MAX_RATIO);
    int ny = screen_center.y - (int16_t)((cos_lookup(trig_angle) * num_radius) / TRIG_MAX_RATIO);
    GSize text_size = graphics_text_layout_get_content_size(
        numbers[i], font, GRect(0, 0, 24, 20),
        GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter);
    GRect text_rect = GRect(nx - text_size.w / 2, ny - text_size.h / 2 - 2,
                            text_size.w, text_size.h);
    graphics_context_set_text_color(ctx, GColorWhite);
    graphics_draw_text(ctx, numbers[i], font, text_rect,
                       GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
  }
}

void outer_ring_draw_tickers(GContext *ctx, GRect bounds, int hour, int minute, int second, bool show_seconds) {
  // Hour ticker
  int hour_deg = (360 * ((hour % 12) * 60 + minute)) / (12 * 60);
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, TICKER_RADIAL_DEPTH,
                       DEG_TO_TRIGANGLE(hour_deg - 4), DEG_TO_TRIGANGLE(hour_deg + 4));
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, TICKER_TIP_SIZE,
                       DEG_TO_TRIGANGLE(hour_deg - 4), DEG_TO_TRIGANGLE(hour_deg + 4));

  // Minute ticker
  int min_deg = (360 * minute) / 60;
  graphics_context_set_fill_color(ctx, GColorCobaltBlue);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, TICKER_RADIAL_DEPTH,
                       DEG_TO_TRIGANGLE(min_deg - 2), DEG_TO_TRIGANGLE(min_deg + 2));
  graphics_context_set_fill_color(ctx, GColorPictonBlue);
  graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, TICKER_TIP_SIZE,
                       DEG_TO_TRIGANGLE(min_deg - 2), DEG_TO_TRIGANGLE(min_deg + 2));

  // Second ticker (only when enabled)
  if (show_seconds) {
    int sec_deg = (360 * second) / 60;
    graphics_context_set_fill_color(ctx, GColorDarkCandyAppleRed);
    graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, TICKER_RADIAL_DEPTH,
                         DEG_TO_TRIGANGLE(sec_deg - 1), DEG_TO_TRIGANGLE(sec_deg + 1));
    graphics_context_set_fill_color(ctx, GColorRed);
    graphics_fill_radial(ctx, bounds, GOvalScaleModeFitCircle, TICKER_TIP_SIZE,
                         DEG_TO_TRIGANGLE(sec_deg - 1), DEG_TO_TRIGANGLE(sec_deg + 1));
  }
}
