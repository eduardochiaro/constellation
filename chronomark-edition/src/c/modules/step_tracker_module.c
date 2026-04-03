#include "step_tracker_module.h"

static BitmapLayer *s_left_icon_layer = NULL;
static BitmapLayer *s_right_icon_layer = NULL;
static GBitmap *s_left_bitmap = NULL;
static GBitmap *s_right_bitmap = NULL;
static int s_step_count = 0;
static int s_step_goal = 8000;
static Layer *s_parent_canvas_layer = NULL;
static int s_last_health_step_count = 0;

static void health_handler(HealthEventType event, void *context) {
  // Update step count when health data changes
  if (event == HealthEventMovementUpdate || event == HealthEventSignificantUpdate) {
    int new_count = (int)health_service_sum_today(HealthMetricStepCount);
    // Only redraw if step count changed meaningfully (by at least 10 steps)
    // to avoid excessive redraws during active movement
    if (new_count - s_last_health_step_count >= 10 || new_count < s_last_health_step_count) {
      s_step_count = new_count;
      s_last_health_step_count = new_count;
      if (s_parent_canvas_layer) {
        layer_mark_dirty(s_parent_canvas_layer);
      }
    }
  }
}

void step_tracker_module_init(Window *window, GRect bounds, Layer *canvas_layer) {
  Layer *window_layer = window_get_root_layer(window);
  s_parent_canvas_layer = canvas_layer;
  
  // Load bitmap resources
  s_left_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WALKING_IMAGE);
  s_right_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FLAG_IMAGE);

  // Compute arc geometry (mirrors canvas_update_proc in constellation.c)
  int radius = 175 / 2 + STEP_TRACK_MARGIN;
  int cy = bounds.size.h / 2;

  // Left icon at 270° (top of arc)
  if (s_left_bitmap) {
    int icon_x = 0 + (bounds.size.w / 2 - radius);
    int icon_y = cy - WALKING_ICON_SIZE;
    s_left_icon_layer = bitmap_layer_create(GRect(icon_x, icon_y, WALKING_ICON_SIZE, WALKING_ICON_SIZE));
    if (s_left_icon_layer) {
      bitmap_layer_set_bitmap(s_left_icon_layer, s_left_bitmap);
      bitmap_layer_set_compositing_mode(s_left_icon_layer, GCompOpSet);
      layer_add_child(window_layer, bitmap_layer_get_layer(s_left_icon_layer));
    }
  }

  // Right icon at 90° (bottom of arc)
  if (s_right_bitmap) {
    int icon_x = bounds.size.w - (bounds.size.w / 2 - radius) - WALKING_ICON_SIZE;
    int icon_y = cy - WALKING_ICON_SIZE;
    s_right_icon_layer = bitmap_layer_create(GRect(icon_x, icon_y, WALKING_ICON_SIZE, WALKING_ICON_SIZE));
    if (s_right_icon_layer) {
      bitmap_layer_set_bitmap(s_right_icon_layer, s_right_bitmap);
      bitmap_layer_set_compositing_mode(s_right_icon_layer, GCompOpSet);
      layer_add_child(window_layer, bitmap_layer_get_layer(s_right_icon_layer));
    }
  }
  
  // Initialize step count
  s_step_count = (int)health_service_sum_today(HealthMetricStepCount);
}

void step_tracker_module_draw(Layer *layer, GContext *ctx, GRect bounds, int radius, GRect arc_bounds) {
  // Calculate progress
  float progress = (s_step_goal > 0) ? ((float)s_step_count / (float)s_step_goal) : 0;
  progress = (progress < 0) ? 0 : (progress > 1.0f) ? 1.0f : progress;
  
  int track_width = STEP_TRACK_WIDTH;

  // Arc style - original curved design
  // Draw base step track (dark gray arc from 90° to 270°)
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_radial(ctx, arc_bounds, GOvalScaleModeFitCircle, track_width,
                        DEG_TO_TRIGANGLE(90), DEG_TO_TRIGANGLE(270));
  
  // Draw step progress
  if (progress > 0) {
    int32_t start_angle = 270 - (int32_t)(180.0f * progress);
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_radial(ctx, arc_bounds, GOvalScaleModeFitCircle, track_width,
                          DEG_TO_TRIGANGLE(start_angle), DEG_TO_TRIGANGLE(270));
  }
}

void step_tracker_module_update(void) {
  s_step_count = (int)health_service_sum_today(HealthMetricStepCount);
  if (s_parent_canvas_layer) {
    layer_mark_dirty(s_parent_canvas_layer);
  }
}

void step_tracker_module_set_goal(int goal) {
  s_step_goal = goal;
  if (s_parent_canvas_layer) {
    layer_mark_dirty(s_parent_canvas_layer);
  }
}

int step_tracker_module_get_count(void) {
  return s_step_count;
}

void step_tracker_module_subscribe(void) {
  health_service_events_subscribe(health_handler, NULL);
  s_step_count = (int)health_service_sum_today(HealthMetricStepCount);
}

void step_tracker_module_unsubscribe(void) {
  health_service_events_unsubscribe();
}

void step_tracker_module_deinit(void) {
  // Destroy bitmap layers
  if (s_left_icon_layer) {
    bitmap_layer_destroy(s_left_icon_layer);
    s_left_icon_layer = NULL;
  }
  if (s_right_icon_layer) {
    bitmap_layer_destroy(s_right_icon_layer);
    s_right_icon_layer = NULL;
  }
  
  // Destroy bitmaps
  if (s_left_bitmap) {
    gbitmap_destroy(s_left_bitmap);
    s_left_bitmap = NULL;
  }
  if (s_right_bitmap) {
    gbitmap_destroy(s_right_bitmap);
    s_right_bitmap = NULL;
  }
}
