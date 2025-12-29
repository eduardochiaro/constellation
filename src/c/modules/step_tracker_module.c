#include "step_tracker_module.h"

static BitmapLayer *s_walk_layer = NULL;
static BitmapLayer *s_flag_layer = NULL;
static GBitmap *s_walking_bitmap = NULL;
static GBitmap *s_flag_bitmap = NULL;
static int s_step_count = 0;
static int s_step_goal = 8000;
static Layer *s_parent_canvas_layer = NULL;

#if defined(PBL_HEALTH)
static void health_handler(HealthEventType event, void *context) {
  // Update step count when health data changes
  if (event == HealthEventMovementUpdate || event == HealthEventSignificantUpdate) {
    s_step_count = (int)health_service_sum_today(HealthMetricStepCount);
    if (s_parent_canvas_layer) {
      layer_mark_dirty(s_parent_canvas_layer);
    }
  }
}
#endif

void step_tracker_module_init(Window *window, GRect bounds, Layer *canvas_layer) {
  Layer *window_layer = window_get_root_layer(window);
  s_parent_canvas_layer = canvas_layer;
  
  // Load bitmap resources
  s_walking_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WALKING_IMAGE);
  s_flag_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FLAG_IMAGE);
  
  // Create walking icon layer
  if (s_walking_bitmap) {
    int icon_x = 5;
    int icon_y = bounds.size.h / 2 - WALKING_ICON_SIZE + 4;
#if defined(PBL_ROUND)
    icon_x += 8;
    icon_y -= 3;
#endif
    s_walk_layer = bitmap_layer_create(GRect(icon_x, icon_y, WALKING_ICON_SIZE, WALKING_ICON_SIZE));
    if (s_walk_layer) {
      bitmap_layer_set_bitmap(s_walk_layer, s_walking_bitmap);
      layer_add_child(window_layer, bitmap_layer_get_layer(s_walk_layer));
    }
  }

  // Create flag icon layer
  if (s_flag_bitmap) {
    int flag_x = bounds.size.w - WALKING_ICON_SIZE - 3;
    int flag_y = bounds.size.h / 2 - WALKING_ICON_SIZE + 4;
#if defined(PBL_ROUND)
    flag_x -= 8;
    flag_y -= 4;
#endif
    s_flag_layer = bitmap_layer_create(GRect(flag_x, flag_y, WALKING_ICON_SIZE, WALKING_ICON_SIZE));
    if (s_flag_layer) {
      bitmap_layer_set_bitmap(s_flag_layer, s_flag_bitmap);
      layer_add_child(window_layer, bitmap_layer_get_layer(s_flag_layer));
    }
  }
  
  // Initialize step count
#if defined(PBL_HEALTH)
  s_step_count = (int)health_service_sum_today(HealthMetricStepCount);
#endif
}

void step_tracker_module_draw(Layer *layer, GContext *ctx, GRect bounds, int radius, GRect arc_bounds, bool use_line_style) {
  // Calculate progress
  float progress = (s_step_goal > 0) ? ((float)s_step_count / (float)s_step_goal) : 0;
  progress = (progress < 0) ? 0 : (progress > 1.0f) ? 1.0f : progress;
  
  if (use_line_style) {
    // Line style - U-shaped perimeter along bottom and sides
    const int line_width = STEP_TRACK_WIDTH;
    const int margin = 3;
    
    // Calculate the perimeter path: left side + bottom + right side
    int left_x = margin;
    int right_x = bounds.size.w - margin - line_width;
    int top_y = bounds.size.h / 2 + 7; // Start from middle-bottom area
    int bottom_y = bounds.size.h - margin - line_width;
    
    int left_height = bottom_y - top_y;
    int bottom_width = right_x - left_x;
    int total_perimeter = left_height + bottom_width + left_height; // left + bottom + right
    
    // Calculate progress distance along perimeter
    int progress_distance = (int)(total_perimeter * progress);
    
    // Draw base perimeter (dark gray)
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    // Left side
    graphics_fill_rect(ctx, GRect(left_x, top_y, line_width, left_height), 0, GCornerNone);
    // Bottom
    graphics_fill_rect(ctx, GRect(left_x, bottom_y, bottom_width + line_width, line_width), 0, GCornerNone);
    // Right side
    graphics_fill_rect(ctx, GRect(right_x, top_y, line_width, left_height), 0, GCornerNone);
    
    // Draw progress (white)
    if (progress > 0) {
      graphics_context_set_fill_color(ctx, GColorWhite);
      
      if (progress_distance <= left_height) {
        // Progress on left side (bottom to top)
        int fill_height = progress_distance;
        graphics_fill_rect(ctx, GRect(left_x, bottom_y - fill_height, line_width, fill_height), 0, GCornerNone);
      } else if (progress_distance <= left_height + bottom_width) {
        // Progress on left side + partial bottom
        graphics_fill_rect(ctx, GRect(left_x, top_y, line_width, left_height), 0, GCornerNone);
        int bottom_progress = progress_distance - left_height;
        graphics_fill_rect(ctx, GRect(left_x, bottom_y, bottom_progress, line_width), 0, GCornerNone);
      } else {
        // Progress on left side + bottom + partial right side
        graphics_fill_rect(ctx, GRect(left_x, top_y, line_width, left_height), 0, GCornerNone);
        graphics_fill_rect(ctx, GRect(left_x, bottom_y, bottom_width + line_width, line_width), 0, GCornerNone);
        int right_progress = progress_distance - left_height - bottom_width;
        graphics_fill_rect(ctx, GRect(right_x, bottom_y - right_progress, line_width, right_progress), 0, GCornerNone);
      }
    }
  } else {
    // Arc style - original curved design
    // Draw base step track (dark gray arc from 90° to 270°)
    graphics_context_set_fill_color(ctx, GColorDarkGray);
    graphics_fill_radial(ctx, arc_bounds, GOvalScaleModeFitCircle, STEP_TRACK_WIDTH,
                         DEG_TO_TRIGANGLE(90), DEG_TO_TRIGANGLE(270));
    
    // Draw step progress
    if (progress > 0) {
      int32_t start_angle = 270 - (int32_t)(180.0f * progress);
      graphics_context_set_fill_color(ctx, GColorWhite);
      graphics_fill_radial(ctx, arc_bounds, GOvalScaleModeFitCircle, STEP_TRACK_WIDTH,
                           DEG_TO_TRIGANGLE(start_angle), DEG_TO_TRIGANGLE(270));
    }
  }
}

void step_tracker_module_update(void) {
#if defined(PBL_HEALTH)
  s_step_count = (int)health_service_sum_today(HealthMetricStepCount);
#endif
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
#if defined(PBL_HEALTH)
  health_service_events_subscribe(health_handler, NULL);
  s_step_count = (int)health_service_sum_today(HealthMetricStepCount);
#endif
}

void step_tracker_module_unsubscribe(void) {
#if defined(PBL_HEALTH)
  health_service_events_unsubscribe();
#endif
}

void step_tracker_module_deinit(void) {
  // Destroy bitmap layers
  if (s_walk_layer) {
    bitmap_layer_destroy(s_walk_layer);
    s_walk_layer = NULL;
  }
  if (s_flag_layer) {
    bitmap_layer_destroy(s_flag_layer);
    s_flag_layer = NULL;
  }
  
  // Destroy bitmaps
  if (s_walking_bitmap) {
    gbitmap_destroy(s_walking_bitmap);
    s_walking_bitmap = NULL;
  }
  if (s_flag_bitmap) {
    gbitmap_destroy(s_flag_bitmap);
    s_flag_bitmap = NULL;
  }
}
