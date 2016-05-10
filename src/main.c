#include <pebble.h>

#define SECOND_RADIUS 7
#define SECOND_LEN PBL_IF_ROUND_ELSE(77, 72)
#define MINUTE_LEN PBL_IF_ROUND_ELSE(80, 78)
#define HOUR_LEN PBL_IF_ROUND_ELSE(53, 53)
#define seconds true
#define fatness PBL_IF_ROUND_ELSE(14, 13)
#define fat(x) (fatness * PBL_IF_ROUND_ELSE(x, x-1))

static Window *window;
static Layer *s_bg_layer, *s_fg_layer, *s_hands_layer;
static TextLayer *s_day_label, *s_bt_label;

static char s_day_buffer[6];

GRect display_bounds;
GPoint center;

bool bt_connected;

static GPoint point_of_polar(int32_t theta, int r) {
  GPoint ret = {
    .x = (int16_t)(sin_lookup(theta) * r / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(theta) * r / TRIG_MAX_RATIO) + center.y,
  };
  
  return ret;
}

static void bg_update_proc(Layer *layer, GContext *ctx) {
  // Draw Background Rings
  GRect frame;
    
  graphics_context_set_fill_color(ctx, GColorVividCerulean);
  graphics_fill_rect(ctx, layer_get_bounds(layer), 0, GCornerNone);
  
  // center dot
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, center, 20);

  frame = grect_inset(display_bounds, GEdgeInsets(fat(2) - 1));
  graphics_context_set_fill_color(ctx, GColorRed);
  graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, fatness + 2, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));

  frame = grect_inset(display_bounds, GEdgeInsets(fat(4) - 1));
  graphics_context_set_fill_color(ctx, GColorIslamicGreen);
  graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, fatness + 2, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
}

static void fg_update_proc(Layer *layer, GContext *ctx) {
  GRect frame;
  
  frame = grect_inset(display_bounds, GEdgeInsets(fat(1)));
  graphics_context_set_fill_color(ctx, GColorBulgarianRose);
  graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, fatness, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
  
  frame = grect_inset(display_bounds, GEdgeInsets(fat(3)));
  graphics_context_set_fill_color(ctx, GColorYellow);
  graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, fatness, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
}

static void hands_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  char *b = s_day_buffer;
  
  // minute hand
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 6);
  graphics_draw_line(ctx, center, point_of_polar(TRIG_MAX_ANGLE * t->tm_min / 60, MINUTE_LEN));

  // hour hand
  graphics_context_set_stroke_width(ctx, 8);
  graphics_draw_line(ctx, center, point_of_polar(TRIG_MAX_ANGLE * (t->tm_hour % 12) / 12, HOUR_LEN));
  
  // second hand
  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, point_of_polar(second_angle, SECOND_LEN), SECOND_RADIUS);

  if (bt_connected) {
    text_layer_set_text(s_bt_label, "");
  } else {
    text_layer_set_text(s_bt_label, "");
  }
  
  strftime(s_day_buffer, sizeof(s_day_buffer), "%d", t);
  if (b[0] == '0') {
    b += 1;
  }
  text_layer_set_text(s_day_label, b);
}


static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_hands_layer);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Background
  s_bg_layer = layer_create(bounds);
  layer_set_update_proc(s_bg_layer, bg_update_proc);
  layer_add_child(window_layer, s_bg_layer);
  
  // Hands
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  layer_add_child(window_layer, s_hands_layer);
  
  // Foreground
  s_fg_layer = layer_create(bounds);
  layer_set_update_proc(s_fg_layer, fg_update_proc);
  layer_add_child(window_layer, s_fg_layer);

  // Day
#ifdef PBL_ROUND
  s_day_label = text_layer_create(GRect(75, 75, 30, 24));
#else
  s_day_label = text_layer_create(GRect(57, 69, 30, 24));
#endif
  text_layer_set_font(s_day_label, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_24)));
  text_layer_set_text_alignment(s_day_label, GTextAlignmentCenter);
  text_layer_set_text(s_day_label, s_day_buffer);
  text_layer_set_background_color(s_day_label, GColorClear);
  text_layer_set_text_color(s_day_label, GColorBlack);
  layer_add_child(s_fg_layer, text_layer_get_layer(s_day_label));

  // Missing phone
#ifdef PBL_ROUND
  s_bt_label = text_layer_create(GRect(26, 50, 52, 64));
#else
  s_bt_label = text_layer_create(GRect(10, 42, 52, 64));
#endif
  text_layer_set_text_alignment(s_bt_label, GTextAlignmentCenter);
  text_layer_set_text(s_bt_label, "");
  text_layer_set_background_color(s_bt_label, GColorClear);
  text_layer_set_text_color(s_bt_label, GColorOxfordBlue);
  text_layer_set_font(s_bt_label, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SYMBOLS_64)));
  layer_add_child(s_fg_layer, text_layer_get_layer(s_bt_label));
}

static void window_unload(Window *window) {
  layer_destroy(s_bg_layer);
  layer_destroy(s_fg_layer);
  layer_destroy(s_hands_layer);

  text_layer_destroy(s_day_label);
}

static void bt_handler(bool connected) {
  bt_connected = connected;
  if (! connected) {
    vibes_double_pulse();
  }
  layer_mark_dirty(s_fg_layer);
}

static void tick_subscribe() {
  if (seconds) {
    tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
  } else {
    tick_timer_service_subscribe(MINUTE_UNIT, handle_second_tick);
  }
}

static void init() {  
  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(window, true);

  s_day_buffer[0] = '\0';

  Layer *window_layer = window_get_root_layer(window);
  display_bounds = layer_get_bounds(window_layer);
  center = grect_center_point(&display_bounds);

  tick_subscribe();
  
  bluetooth_connection_service_subscribe(bt_handler);
  bt_connected = bluetooth_connection_service_peek();
}

static void deinit() {
  // XXX: text destroy?

  tick_timer_service_unsubscribe();
  window_destroy(window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
