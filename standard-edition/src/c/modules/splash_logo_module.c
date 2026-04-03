#include "splash_logo_module.h"
#include "../utilities/logos.h"

static BitmapLayer *s_splash_logo_layer;
static GBitmap *s_splash_bitmap;

void splash_logo_init(void) {
  s_splash_bitmap = NULL;
  s_splash_logo_layer = NULL;
}

void splash_logo_show(Window *window, int style) {
  if (style < 1 || style > SPLASH_LOGO_COUNT) return;

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_splash_bitmap = gbitmap_create_with_resource(s_logo_resources[style - 1]);
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

static void splash_logo_hide(void) {
  if (s_splash_logo_layer) {
    bitmap_layer_destroy(s_splash_logo_layer);
    s_splash_logo_layer = NULL;
  }
  if (s_splash_bitmap) {
    gbitmap_destroy(s_splash_bitmap);
    s_splash_bitmap = NULL;
  }
}

void splash_logo_cleanup(void) {
  splash_logo_hide();
}