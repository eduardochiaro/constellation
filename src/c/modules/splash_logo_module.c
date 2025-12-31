#include "splash_logo_module.h"


static BitmapLayer *s_splash_logo_layer;
static GBitmap *s_cbw_logo_bitmap;
static GBitmap *s_ccolor_logo_bitmap;
static GBitmap *s_housevarun_logo_bitmap;
static GBitmap *s_freestar_logo_bitmap;
static GBitmap *s_sysdef_logo_bitmap;
static GBitmap *s_crimson_logo_bitmap;

void splash_logo_init(void) {

  s_cbw_logo_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CONSTELLATION_BW_LOGO_IMAGE);
  s_ccolor_logo_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CONSTELLATION_COLOR_LOGO_IMAGE);
  s_housevarun_logo_bitmap = gbitmap_create_with_resource(RESOURCE_ID_HOUSEVARUUN_LOGO_IMAGE);
  s_freestar_logo_bitmap = gbitmap_create_with_resource(RESOURCE_ID_FREESTAR_LOGO_IMAGE);
  s_sysdef_logo_bitmap = gbitmap_create_with_resource(RESOURCE_ID_SYSDEF_LOGO_IMAGE);
  s_crimson_logo_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CRIMSON_LOGO_IMAGE);
}

void splash_logo_show(Window *window, char *s_style_logo) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

// Show splash logo (color or B&W based on user setting)
    GBitmap *splash_logo = NULL;
    if (strcmp(s_style_logo, "color") == 0) {
      splash_logo = s_ccolor_logo_bitmap;
    } else if (strcmp(s_style_logo, "bw") == 0) {
      splash_logo = s_cbw_logo_bitmap;
    } else if (strcmp(s_style_logo, "house_varuun") == 0) {
      splash_logo = s_housevarun_logo_bitmap;
    } else if (strcmp(s_style_logo, "freestar") == 0) {
      splash_logo = s_freestar_logo_bitmap;
    } else if (strcmp(s_style_logo, "sysdef") == 0) {
      splash_logo = s_sysdef_logo_bitmap;
    } else if (strcmp(s_style_logo, "crimson") == 0) {
      splash_logo = s_crimson_logo_bitmap;
    }
    
    if (splash_logo) {
      GRect logo_bounds = gbitmap_get_bounds(splash_logo);
      int x = (bounds.size.w - logo_bounds.size.w) / 2;
      int y = (bounds.size.h - logo_bounds.size.h) / 2;
      
      s_splash_logo_layer = bitmap_layer_create(GRect(x, y, logo_bounds.size.w, logo_bounds.size.h));
      if (s_splash_logo_layer) {
        bitmap_layer_set_bitmap(s_splash_logo_layer, splash_logo);
        layer_add_child(window_layer, bitmap_layer_get_layer(s_splash_logo_layer));
      }
    }
}

void splash_logo_hide(void) {
  if (s_splash_logo_layer) {
    bitmap_layer_destroy(s_splash_logo_layer);
    s_splash_logo_layer = NULL;
  }
}

void splash_logo_cleanup(void) {
  if (s_cbw_logo_bitmap) {
    gbitmap_destroy(s_cbw_logo_bitmap);
    s_cbw_logo_bitmap = NULL;
  }
  if (s_ccolor_logo_bitmap) {
    gbitmap_destroy(s_ccolor_logo_bitmap);
    s_ccolor_logo_bitmap = NULL;
  }
  if (s_housevarun_logo_bitmap) {
    gbitmap_destroy(s_housevarun_logo_bitmap);
    s_housevarun_logo_bitmap = NULL;
  }
  if (s_freestar_logo_bitmap) {
    gbitmap_destroy(s_freestar_logo_bitmap);
    s_freestar_logo_bitmap = NULL;
  }
  if (s_sysdef_logo_bitmap) {
    gbitmap_destroy(s_sysdef_logo_bitmap);
    s_sysdef_logo_bitmap = NULL;
  }
  if (s_crimson_logo_bitmap) {
    gbitmap_destroy(s_crimson_logo_bitmap);
    s_crimson_logo_bitmap = NULL;
  }
}