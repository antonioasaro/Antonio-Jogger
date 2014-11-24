#include <pebble.h>

static Window *window;
static TextLayer *text_layer;
static int count = 0;
static int incr  = 0;

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(text_layer, "Select");
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  incr = 1;
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  incr = 0;
}

void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  count = 0;
}

void select_long_click_release_handler(ClickRecognizerRef recognizer, void *context) {
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 1000, select_long_click_handler, select_long_click_release_handler);

}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  text_layer = text_layer_create((GRect) { .origin = { 0, 48 }, .size = { bounds.size.w, 80 } });
  text_layer_set_text(text_layer, "Jogger");
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_BITHAM_30_BLACK));
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(text_layer);
}

void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
static char str[] = "0:00:00";  
	count = count + incr;
    str[0] = 48 + (count / 60*10*6) % 10;
    str[2] = 48 + (count / 60*10) % 6;
    str[3] = 48 + (count / 60) % 10;
    str[5] = 48 + (count / 10) % 6;
    str[6] = 48 + (count % 10);
	if ((count > 1) && (((count % 60*10) == 0) || ((count % 60*11) == 0))) vibes_long_pulse(); 
	text_layer_set_text(text_layer, str);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
	.load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
	
}

static void deinit(void) {
  window_destroy(window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}