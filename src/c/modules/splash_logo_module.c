#include "splash_logo_module.h"


static BitmapLayer *s_splash_logo_layer;
static GBitmap *s_splash_bitmap;

void splash_logo_init(void) {
  s_splash_bitmap = NULL;
  s_splash_logo_layer = NULL;
}

void splash_logo_show(Window *window, char *s_style_logo) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Load only the selected logo bitmap
  uint32_t resource_id = 0;
  if (strcmp(s_style_logo, "color") == 0) {
    resource_id = RESOURCE_ID_CONSTELLATION_COLOR_LOGO_IMAGE;
  } else if (strcmp(s_style_logo, "bw") == 0) {
    resource_id = RESOURCE_ID_CONSTELLATION_BW_LOGO_IMAGE;
  } else if (strcmp(s_style_logo, "house_varuun") == 0) {
    resource_id = RESOURCE_ID_HOUSEVARUUN_LOGO_IMAGE;
  } else if (strcmp(s_style_logo, "freestar") == 0) {
    resource_id = RESOURCE_ID_FREESTAR_LOGO_IMAGE;
  } else if (strcmp(s_style_logo, "sysdef") == 0) {
    resource_id = RESOURCE_ID_SYSDEF_LOGO_IMAGE;
  } else if (strcmp(s_style_logo, "crimson") == 0) {
    resource_id = RESOURCE_ID_CRIMSON_LOGO_IMAGE;
  }

  if (resource_id == 0) return;

  s_splash_bitmap = gbitmap_create_with_resource(resource_id);
  if (!s_splash_bitmap) return;

  GRect logo_bounds = gbitmap_get_bounds(s_splash_bitmap);
  int x = (bounds.size.w - logo_bounds.size.w) / 2;
  int y = (bounds.size.h - logo_bounds.size.h) / 2;

  s_splash_logo_layer = bitmap_layer_create(GRect(x, y, logo_bounds.size.w, logo_bounds.size.h));
  if (s_splash_logo_layer) {
    bitmap_layer_set_bitmap(s_splash_logo_layer, s_splash_bitmap);
    bitmap_layer_set_compositing_mode(s_splash_logo_layer, GCompOpSet);
    layer_add_child(window_layer, bitmap_layer_get_layer(s_splash_logo_layer));
  }
}

void splash_logo_hide(void) {
  if (s_splash_logo_layer) {
    bitmap_layer_destroy(s_splash_logo_layer);
    s_splash_logo_layer = NULL;
  }
}

void splash_logo_cleanup(void) {
  splash_logo_hide();
  if (s_splash_bitmap) {
    gbitmap_destroy(s_splash_bitmap);
    s_splash_bitmap = NULL;
  }
}